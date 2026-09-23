#include "printer_service.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "cJSON.h"
#include "esp_err.h"
#include "esp_http_client.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "mqtt_client.h"

#include "esp_bsp.h"
#include "printer_config.h"
#include "printer_thumbnail.h"
#include "printer_thumbnail_fetch.h"
#include "pulsemon_settings.h"
#include "vars.h"
#include "wifi_manager.h"

#define PRINTER_EVENT_CONNECTED BIT0
#define PRINTER_EVENT_REGISTERED BIT1
#define PRINTER_EVENT_ATTRIBUTES BIT2
#define PRINTER_EVENT_STATUS BIT3
#define PRINTER_EVENT_AVAILABLE BIT4
#define PRINTER_EVENT_THUMBNAIL BIT5
#define PRINTER_EVENT_STOP BIT6
#define PRINTER_EVENT_WAKE BIT7

#define PRINTER_HTTP_BODY_CAP 2048
#define PRINTER_MQTT_RX_MAX (256U * 1024U)
#define PRINTER_TOPIC_CAP 192
#define PRINTER_SERIAL_CAP 64
#define PRINTER_CLIENT_ID_CAP 64
#define PRINTER_REQUEST_PREFIX_CAP 80
#define PRINTER_URI_CAP 128
#define PRINTER_TIME_VALID_EPOCH_MIN 1609459200LL
#define PRINTER_FILENAME_CAP 256
#define PRINTER_THUMBNAIL_RETRY_MS 30000

static const char *TAG = "printer";

static TaskHandle_t s_task;
static EventGroupHandle_t s_events;
static esp_mqtt_client_handle_t s_mqtt;

static char s_serial[PRINTER_SERIAL_CAP];
static char s_client_id[PRINTER_CLIENT_ID_CAP];
static char s_request_prefix[PRINTER_REQUEST_PREFIX_CAP];
static char s_topic_request[PRINTER_TOPIC_CAP];
static char s_topic_response[PRINTER_TOPIC_CAP];
static char s_topic_register_response[PRINTER_TOPIC_CAP];
static char s_topic_all[PRINTER_TOPIC_CAP];
static char s_mqtt_uri[PRINTER_URI_CAP];

static char s_rx_topic[PRINTER_TOPIC_CAP];
static char *s_rx_dynamic;
static size_t s_rx_dynamic_cap;
static int s_subscribe_msg_id = -1;
static int s_request_id = 0;
static int s_pending_attributes_id = -1;
static int s_pending_status_id = -1;
static int s_pending_thumbnail_id = -1;
static char s_pending_thumbnail_filename[PRINTER_FILENAME_CAP];
static printer_thumbnail_fetch_result_t s_thumbnail_request_result = THUMBNAIL_FETCH_RETRY;
static bool s_attributes_loaded;
static char s_job_filename[PRINTER_FILENAME_CAP];
static bool s_thumbnail_pending;
static bool s_thumbnail_clear_pending;
static bool s_service_requested;
static char s_thumbnail_last_attempt_filename[PRINTER_FILENAME_CAP];
static int64_t s_thumbnail_last_attempt_us;
static portMUX_TYPE s_state_mux = portMUX_INITIALIZER_UNLOCKED;


static void state_reset_requests(void)
{
    portENTER_CRITICAL(&s_state_mux);
    s_pending_attributes_id = -1;
    s_pending_status_id = -1;
    s_pending_thumbnail_id = -1;
    s_pending_thumbnail_filename[0] = '\0';
    s_thumbnail_request_result = THUMBNAIL_FETCH_RETRY;
    s_attributes_loaded = false;
    portEXIT_CRITICAL(&s_state_mux);
}

static bool state_service_requested(void)
{
    portENTER_CRITICAL(&s_state_mux);
    bool requested = s_service_requested;
    portEXIT_CRITICAL(&s_state_mux);
    return requested;
}

static void state_set_service_requested(bool requested)
{
    portENTER_CRITICAL(&s_state_mux);
    s_service_requested = requested;
    portEXIT_CRITICAL(&s_state_mux);
}

static bool state_attributes_loaded(void)
{
    portENTER_CRITICAL(&s_state_mux);
    bool loaded = s_attributes_loaded;
    portEXIT_CRITICAL(&s_state_mux);
    return loaded;
}

static void state_set_attributes_loaded(bool loaded)
{
    portENTER_CRITICAL(&s_state_mux);
    s_attributes_loaded = loaded;
    portEXIT_CRITICAL(&s_state_mux);
}

static void state_set_pending_attributes(int request_id)
{
    portENTER_CRITICAL(&s_state_mux);
    s_pending_attributes_id = request_id;
    portEXIT_CRITICAL(&s_state_mux);
}

static void state_set_pending_status(int request_id)
{
    portENTER_CRITICAL(&s_state_mux);
    s_pending_status_id = request_id;
    portEXIT_CRITICAL(&s_state_mux);
}


static void state_set_pending_thumbnail(int request_id, const char *filename)
{
    portENTER_CRITICAL(&s_state_mux);
    s_pending_thumbnail_id = request_id;
    snprintf(s_pending_thumbnail_filename, sizeof(s_pending_thumbnail_filename), "%s", filename != NULL ? filename : "");
    s_thumbnail_request_result = THUMBNAIL_FETCH_RETRY;
    portEXIT_CRITICAL(&s_state_mux);
}

static int state_get_pending_thumbnail(char *filename, size_t filename_len)
{
    portENTER_CRITICAL(&s_state_mux);
    int request_id = s_pending_thumbnail_id;
    if (filename != NULL && filename_len > 0) {
        snprintf(filename, filename_len, "%s", s_pending_thumbnail_filename);
    }
    portEXIT_CRITICAL(&s_state_mux);
    return request_id;
}

static void state_set_thumbnail_request_result(printer_thumbnail_fetch_result_t result)
{
    portENTER_CRITICAL(&s_state_mux);
    s_thumbnail_request_result = result;
    s_pending_thumbnail_id = -1;
    s_pending_thumbnail_filename[0] = '\0';
    portEXIT_CRITICAL(&s_state_mux);
}

static printer_thumbnail_fetch_result_t state_get_thumbnail_request_result(void)
{
    portENTER_CRITICAL(&s_state_mux);
    printer_thumbnail_fetch_result_t result = s_thumbnail_request_result;
    portEXIT_CRITICAL(&s_state_mux);
    return result;
}

static bool state_thumbnail_filename_is_current(const char *filename)
{
    if (filename == NULL) {
        return false;
    }
    portENTER_CRITICAL(&s_state_mux);
    bool current = strncmp(s_job_filename, filename, sizeof(s_job_filename)) == 0;
    portEXIT_CRITICAL(&s_state_mux);
    return current;
}

static void state_get_pending(int *attributes_id, int *status_id)
{
    portENTER_CRITICAL(&s_state_mux);
    if (attributes_id != NULL) {
        *attributes_id = s_pending_attributes_id;
    }
    if (status_id != NULL) {
        *status_id = s_pending_status_id;
    }
    portEXIT_CRITICAL(&s_state_mux);
}

static void state_update_job_filename(const char *filename)
{
    char next[PRINTER_FILENAME_CAP] = {0};
    if (filename != NULL) {
        snprintf(next, sizeof(next), "%s", filename);
    }

    portENTER_CRITICAL(&s_state_mux);
    bool changed = false;
    for (size_t i = 0; i < sizeof(s_job_filename); ++i) {
        if (s_job_filename[i] != next[i]) {
            changed = true;
            break;
        }
        if (next[i] == '\0') {
            break;
        }
    }
    if (changed) {
        memcpy(s_job_filename, next, sizeof(s_job_filename));
        s_thumbnail_clear_pending = true;
        s_thumbnail_pending = next[0] != '\0';
    }
    portEXIT_CRITICAL(&s_state_mux);

    if (changed) {
        ESP_LOGI(TAG,
                 "printer job filename changed file=%s thumbnail_pending=%d",
                 next[0] != '\0' ? next : "<none>",
                 next[0] != '\0' ? 1 : 0);
    }
}

static void state_get_thumbnail_work(char *filename, size_t filename_len, bool *pending, bool *clear_pending)
{
    portENTER_CRITICAL(&s_state_mux);
    if (filename != NULL && filename_len > 0) {
        size_t copy_len = filename_len < sizeof(s_job_filename) ? filename_len : sizeof(s_job_filename);
        memcpy(filename, s_job_filename, copy_len);
        filename[copy_len - 1] = '\0';
    }
    if (pending != NULL) {
        *pending = s_thumbnail_pending;
    }
    if (clear_pending != NULL) {
        *clear_pending = s_thumbnail_clear_pending;
    }
    portEXIT_CRITICAL(&s_state_mux);
}

static void state_thumbnail_clear_done(void)
{
    portENTER_CRITICAL(&s_state_mux);
    s_thumbnail_clear_pending = false;
    portEXIT_CRITICAL(&s_state_mux);
}

static void state_thumbnail_fetch_done(const char *filename)
{
    if (filename == NULL) {
        return;
    }

    char expected[PRINTER_FILENAME_CAP] = {0};
    snprintf(expected, sizeof(expected), "%s", filename);

    portENTER_CRITICAL(&s_state_mux);
    bool same = true;
    for (size_t i = 0; i < sizeof(s_job_filename); ++i) {
        if (s_job_filename[i] != expected[i]) {
            same = false;
            break;
        }
        if (expected[i] == '\0') {
            break;
        }
    }
    if (same) {
        s_thumbnail_pending = false;
    }
    portEXIT_CRITICAL(&s_state_mux);
}

static bool state_reset_job(void)
{
    portENTER_CRITICAL(&s_state_mux);
    bool had_job = s_job_filename[0] != '\0' || s_thumbnail_pending || s_thumbnail_clear_pending;
    s_job_filename[0] = '\0';
    s_thumbnail_pending = false;
    s_thumbnail_clear_pending = false;
    s_thumbnail_last_attempt_filename[0] = '\0';
    s_thumbnail_last_attempt_us = 0;
    portEXIT_CRITICAL(&s_state_mux);
    return had_job;
}

static void printer_set_available(bool available)
{
    if (s_events == NULL) {
        return;
    }

    bool was_available = (xEventGroupGetBits(s_events) & PRINTER_EVENT_AVAILABLE) != 0;
    if (available) {
        xEventGroupSetBits(s_events, PRINTER_EVENT_AVAILABLE);
    } else {
        xEventGroupClearBits(s_events, PRINTER_EVENT_AVAILABLE);
    }
    if (was_available != available) {
        ESP_LOGI(TAG, "printer availability=%d", available ? 1 : 0);
    }
}

bool printer_service_is_available(void)
{
    if (s_events == NULL) {
        return false;
    }
    return (xEventGroupGetBits(s_events) & PRINTER_EVENT_AVAILABLE) != 0;
}

static bool printer_configured(void)
{
    return PRINTER_HOST[0] != '\0' && PRINTER_ACCESS_CODE[0] != '\0';
}

static bool wifi_connected(void)
{
    pulsemon_wifi_status_t status = {0};
    pulsemon_wifi_manager_get_status(&status);
    return status.connected;
}

static cJSON *json_object(cJSON *parent, const char *name)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(parent, name);
    return cJSON_IsObject(item) ? item : NULL;
}

static const char *json_string(cJSON *parent, const char *name)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(parent, name);
    return cJSON_IsString(item) ? item->valuestring : NULL;
}

static uint32_t json_u32(cJSON *parent, const char *name, uint32_t fallback)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(parent, name);
    if (!cJSON_IsNumber(item) || item->valuedouble < 0.0) {
        return fallback;
    }
    if (item->valuedouble > 4294967295.0) {
        return UINT32_MAX;
    }
    return (uint32_t)item->valuedouble;
}

static int json_int(cJSON *parent, const char *name, int fallback)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(parent, name);
    return cJSON_IsNumber(item) ? item->valueint : fallback;
}

static void format_duration(uint32_t seconds, char *out, size_t out_len)
{
    if (out == NULL || out_len == 0) {
        return;
    }
    uint32_t hours = seconds / 3600U;
    uint32_t minutes = (seconds % 3600U) / 60U;
    uint32_t secs = seconds % 60U;
    snprintf(out, out_len, "%lu:%02lu:%02lu",
             (unsigned long)hours,
             (unsigned long)minutes,
             (unsigned long)secs);
}

static bool format_clock(time_t epoch, char *out, size_t out_len)
{
    if (out == NULL || out_len == 0 || epoch < PRINTER_TIME_VALID_EPOCH_MIN) {
        return false;
    }

    pulsemon_settings_t settings;
    int16_t gmt_offset_min = 0;
    esp_err_t err = pulsemon_settings_load(&settings);
    if (err == ESP_OK || err == ESP_ERR_INVALID_SIZE) {
        gmt_offset_min = settings.gmt_offset_min;
    }

    time_t adjusted = epoch + ((time_t)gmt_offset_min * 60);
    struct tm tm_value;
    if (gmtime_r(&adjusted, &tm_value) == NULL) {
        return false;
    }

    snprintf(out, out_len, "%02d:%02d", tm_value.tm_hour, tm_value.tm_min);
    return true;
}

static void apply_unavailable_values(void)
{
    if (!bsp_display_lock(pdMS_TO_TICKS(100))) {
        return;
    }
    set_var_name_printer("--");
    set_var_printer_ip("--");
    set_var_print_file_name("--");
    set_var_print_time_start("--:--");
    set_var_print_time_end("--:--");
    set_var_print_time_elapsed("00:00:00");
    set_var_print_time_remaining("00:00:00");
    set_var_print_bar(0);
    bsp_display_unlock();
}

static bool apply_status_result(cJSON *result)
{
    cJSON *machine_status = json_object(result, "machine_status");
    cJSON *print_status = json_object(result, "print_status");
    if (machine_status == NULL || print_status == NULL) {
        return false;
    }

    int progress = json_int(machine_status, "progress", 0);
    const char *filename = json_string(print_status, "filename");
    uint32_t elapsed = json_u32(print_status, "print_duration", 0);
    uint32_t remaining = json_u32(print_status, "remaining_time_sec", 0);

    char elapsed_buf[32];
    char remaining_buf[32];
    char start_buf[16] = "--:--";
    char end_buf[16] = "--:--";
    format_duration(elapsed, elapsed_buf, sizeof(elapsed_buf));
    format_duration(remaining, remaining_buf, sizeof(remaining_buf));

    bool job_present = filename != NULL && filename[0] != '\0';
    state_update_job_filename(job_present ? filename : NULL);
    if (job_present) {
        time_t now = 0;
        time(&now);
        (void)format_clock(now - (time_t)elapsed, start_buf, sizeof(start_buf));
        (void)format_clock(now + (time_t)remaining, end_buf, sizeof(end_buf));
    }

    if (bsp_display_lock(pdMS_TO_TICKS(100))) {
        set_var_printer_ip(PRINTER_HOST);
        set_var_print_file_name(job_present ? filename : "--");
        set_var_print_time_start(job_present ? start_buf : "--:--");
        set_var_print_time_end(job_present ? end_buf : "--:--");
        set_var_print_time_elapsed(job_present ? elapsed_buf : "00:00:00");
        set_var_print_time_remaining(job_present ? remaining_buf : "00:00:00");
        set_var_print_bar(job_present ? progress : 0);
        bsp_display_unlock();
    }

    printer_set_available(true);
    return true;
}

static bool apply_attributes_result(cJSON *result)
{
    const char *name = json_string(result, "hostname");
    if (name == NULL || name[0] == '\0') {
        name = json_string(result, "machine_model");
    }
    if (name == NULL || name[0] == '\0') {
        return false;
    }

    if (bsp_display_lock(pdMS_TO_TICKS(100))) {
        set_var_name_printer(name);
        set_var_printer_ip(PRINTER_HOST);
        bsp_display_unlock();
    }
    return true;
}

typedef struct {
    char *buf;
    size_t cap;
    size_t len;
    bool overflow;
} http_acc_t;

static esp_err_t printer_http_event(esp_http_client_event_t *evt)
{
    http_acc_t *acc = (http_acc_t *)evt->user_data;
    if (acc == NULL) {
        return ESP_OK;
    }
    if (evt->event_id != HTTP_EVENT_ON_DATA || evt->data == NULL || evt->data_len <= 0) {
        return ESP_OK;
    }

    size_t incoming = (size_t)evt->data_len;
    if (acc->len + incoming >= acc->cap) {
        acc->overflow = true;
        return ESP_OK;
    }

    memcpy(acc->buf + acc->len, evt->data, incoming);
    acc->len += incoming;
    acc->buf[acc->len] = '\0';
    return ESP_OK;
}

static bool fetch_serial_number(void)
{
    char url[256];
    snprintf(url, sizeof(url), "http://%s:%u/system/info?X-Token=%s",
             PRINTER_HOST,
             (unsigned)PRINTER_HTTP_PORT,
             PRINTER_ACCESS_CODE);

    char body[PRINTER_HTTP_BODY_CAP] = {0};
    http_acc_t acc = {
        .buf = body,
        .cap = sizeof(body),
        .len = 0,
        .overflow = false,
    };
    esp_http_client_config_t cfg = {
        .url = url,
        .timeout_ms = PRINTER_HTTP_TIMEOUT_MS,
        .event_handler = printer_http_event,
        .user_data = &acc,
        .buffer_size = 1024,
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (client == NULL) {
        return false;
    }

    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    if (err != ESP_OK || status != 200 || acc.overflow || acc.len == 0) {
        ESP_LOGD(TAG, "bootstrap failed err=%s status=%d", esp_err_to_name(err), status);
        return false;
    }

    cJSON *root = cJSON_Parse(body);
    if (!cJSON_IsObject(root)) {
        if (root != NULL) {
            cJSON_Delete(root);
        }
        return false;
    }
    cJSON *system_info = json_object(root, "system_info");
    const char *sn = system_info != NULL ? json_string(system_info, "sn") : NULL;
    bool ok = sn != NULL && sn[0] != '\0' && strlen(sn) < sizeof(s_serial);
    if (ok) {
        snprintf(s_serial, sizeof(s_serial), "%s", sn);
    }
    cJSON_Delete(root);
    return ok;
}

static bool publish_thumbnail_request(int request_id, const char *filename);
static printer_thumbnail_fetch_result_t request_thumbnail(const char *filename);

static void refresh_printer_thumbnail(void)
{
    char filename[PRINTER_FILENAME_CAP] = {0};
    bool pending = false;
    bool clear_pending = false;
    state_get_thumbnail_work(filename, sizeof(filename), &pending, &clear_pending);

    if (clear_pending) {
        esp_err_t clear_err = printer_thumbnail_clear();
        if (clear_err == ESP_OK) {
            state_thumbnail_clear_done();
        } else {
            ESP_LOGW(TAG, "printer thumbnail clear failed err=%s", esp_err_to_name(clear_err));
            return;
        }
    }
    if (!pending || filename[0] == '\0') {
        return;
    }

    int64_t now_us = esp_timer_get_time();
    bool same_attempt = strcmp(s_thumbnail_last_attempt_filename, filename) == 0;
    if (same_attempt && s_thumbnail_last_attempt_us != 0 &&
        now_us - s_thumbnail_last_attempt_us < (int64_t)PRINTER_THUMBNAIL_RETRY_MS * 1000LL) {
        return;
    }

    snprintf(s_thumbnail_last_attempt_filename, sizeof(s_thumbnail_last_attempt_filename), "%s", filename);
    s_thumbnail_last_attempt_us = now_us;

    ESP_LOGI(TAG, "printer thumbnail MQTT request file=%s method=1045", filename);
    printer_thumbnail_fetch_result_t result = request_thumbnail(filename);
    if (result == THUMBNAIL_FETCH_OK) {
        state_thumbnail_fetch_done(filename);
        return;
    }
    if (result == THUMBNAIL_FETCH_NO_IMAGE || result == THUMBNAIL_FETCH_NOT_FOUND) {
        state_thumbnail_fetch_done(filename);
        ESP_LOGI(TAG, "printer thumbnail unavailable file=%s result=%d", filename, (int)result);
        return;
    }

    ESP_LOGW(TAG,
             "printer thumbnail MQTT transient failure file=%s retry_ms=%u result=%d",
             filename,
             (unsigned)PRINTER_THUMBNAIL_RETRY_MS,
             (int)result);
}

static bool topic_equals(const char *topic, const char *expected)
{
    return topic != NULL && expected != NULL && strcmp(topic, expected) == 0;
}

static void process_mqtt_message(const char *topic, const char *payload)
{
    if (topic == NULL || payload == NULL) {
        return;
    }

    cJSON *root = cJSON_Parse(payload);
    if (!cJSON_IsObject(root)) {
        if (root != NULL) {
            cJSON_Delete(root);
        }
        return;
    }

    if (topic_equals(topic, s_topic_register_response)) {
        const char *error = json_string(root, "error");
        if (error == NULL || strcmp(error, "ok") == 0) {
            ESP_LOGI(TAG, "printer MQTT registered");
            xEventGroupSetBits(s_events, PRINTER_EVENT_REGISTERED);
        }
        cJSON_Delete(root);
        return;
    }

    if (!topic_equals(topic, s_topic_response)) {
        cJSON_Delete(root);
        return;
    }

    int method = json_int(root, "method", -1);
    if (method == 6000 || method == 6008) {
        /* Unsolicited partial status push. Full method-1002 polling owns this UI. */
        cJSON_Delete(root);
        return;
    }

    cJSON *id_item = cJSON_GetObjectItemCaseSensitive(root, "id");
    cJSON *result = json_object(root, "result");
    if (!cJSON_IsNumber(id_item) || result == NULL) {
        cJSON_Delete(root);
        return;
    }

    int request_id = id_item->valueint;
    int error_code = json_int(result, "error_code", 0);
    char pending_thumbnail_filename[PRINTER_FILENAME_CAP] = {0};
    int pending_thumbnail_id = state_get_pending_thumbnail(pending_thumbnail_filename, sizeof(pending_thumbnail_filename));

    if (request_id == pending_thumbnail_id) {
        printer_thumbnail_fetch_result_t thumb_result = THUMBNAIL_FETCH_RETRY;
        if (!state_thumbnail_filename_is_current(pending_thumbnail_filename)) {
            ESP_LOGI(TAG,
                     "thumbnail method=1045 stale response ignored file=%s",
                     pending_thumbnail_filename[0] != '\0' ? pending_thumbnail_filename : "<unknown>");
            thumb_result = THUMBNAIL_FETCH_NO_IMAGE;
        } else if (error_code != 0) {
            ESP_LOGW(TAG,
                     "thumbnail method=1045 request id=%d error_code=%d",
                     request_id,
                     error_code);
            thumb_result = (error_code == 1003) ? THUMBNAIL_FETCH_NOT_FOUND : THUMBNAIL_FETCH_NO_IMAGE;
        } else {
            const char *thumbnail = json_string(result, "thumbnail");
            if (thumbnail == NULL || thumbnail[0] == '\0') {
                ESP_LOGW(TAG, "thumbnail method=1045 response has no thumbnail field");
                thumb_result = THUMBNAIL_FETCH_NO_IMAGE;
            } else {
                uint8_t *png_data = NULL;
                size_t png_len = 0;
                uint16_t width = 0;
                uint16_t height = 0;
                thumb_result = printer_thumbnail_decode_base64_png(thumbnail,
                                                                   &png_data,
                                                                   &png_len,
                                                                   &width,
                                                                   &height);
                if (thumb_result == THUMBNAIL_FETCH_OK) {
                    esp_err_t ui_err = printer_thumbnail_set_png(png_data, png_len, width, height);
                    if (ui_err == ESP_OK) {
                        ESP_LOGI(TAG,
                                 "printer thumbnail cached via MQTT file=%s size=%ux%u bytes=%u",
                                 pending_thumbnail_filename[0] != '\0' ? pending_thumbnail_filename : "<unknown>",
                                 (unsigned)width,
                                 (unsigned)height,
                                 (unsigned)png_len);
                    } else {
                        ESP_LOGW(TAG,
                                 "printer thumbnail UI update failed err=%s",
                                 esp_err_to_name(ui_err));
                        thumb_result = THUMBNAIL_FETCH_RETRY;
                    }
                }
            }
        }
        state_set_thumbnail_request_result(thumb_result);
        xEventGroupSetBits(s_events, PRINTER_EVENT_THUMBNAIL);
        cJSON_Delete(root);
        return;
    }

    if (error_code != 0) {
        ESP_LOGW(TAG, "request id=%d error_code=%d", request_id, error_code);
        cJSON_Delete(root);
        return;
    }

    int pending_attributes_id = -1;
    int pending_status_id = -1;
    state_get_pending(&pending_attributes_id, &pending_status_id);

    if (request_id == pending_attributes_id) {
        (void)apply_attributes_result(result);
        state_set_attributes_loaded(true);
        xEventGroupSetBits(s_events, PRINTER_EVENT_ATTRIBUTES);
    } else if (request_id == pending_status_id) {
        if (apply_status_result(result)) {
            xEventGroupSetBits(s_events, PRINTER_EVENT_STATUS);
        }
    }

    cJSON_Delete(root);
}

static void publish_register(void)
{
    char payload[192];
    snprintf(payload, sizeof(payload),
             "{\"client_id\":\"%s\",\"request_id\":\"%s\"}",
             s_client_id,
             s_request_prefix);

    char topic[PRINTER_TOPIC_CAP];
    snprintf(topic, sizeof(topic), "elegoo/%s/api_register", s_serial);
    esp_mqtt_client_publish(s_mqtt, topic, payload, 0, 0, 0);
}

static int next_request_id(void)
{
    s_request_id++;
    if (s_request_id <= 0) {
        s_request_id = 1;
    }
    return s_request_id;
}

static bool publish_request(int method, int request_id)
{
    char payload[128];
    snprintf(payload, sizeof(payload), "{\"id\":%d,\"method\":%d,\"params\":{}}", request_id, method);
    return esp_mqtt_client_publish(s_mqtt, s_topic_request, payload, 0, 0, 0) >= 0;
}

static bool publish_thumbnail_request(int request_id, const char *filename)
{
    if (filename == NULL || filename[0] == '\0') {
        return false;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON *params = cJSON_CreateObject();
    if (root == NULL || params == NULL ||
        !cJSON_AddNumberToObject(root, "id", request_id) ||
        !cJSON_AddNumberToObject(root, "method", 1045) ||
        !cJSON_AddItemToObject(root, "params", params) ||
        !cJSON_AddStringToObject(params, "storage_media", "local") ||
        !cJSON_AddStringToObject(params, "file_name", filename)) {
        if (root != NULL) {
            cJSON_Delete(root);
        } else if (params != NULL) {
            cJSON_Delete(params);
        }
        return false;
    }

    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (payload == NULL) {
        return false;
    }

    int msg_id = esp_mqtt_client_publish(s_mqtt, s_topic_request, payload, 0, 0, 0);
    cJSON_free(payload);
    return msg_id >= 0;
}

static void publish_app_ping(void)
{
    (void)esp_mqtt_client_publish(s_mqtt, s_topic_request, "{\"type\":\"PING\"}", 0, 0, 0);
}

static void mqtt_rx_reset(void)
{
    if (s_rx_dynamic != NULL) {
        free(s_rx_dynamic);
        s_rx_dynamic = NULL;
    }
    s_rx_dynamic_cap = 0;
}

static void mqtt_data_event(esp_mqtt_event_handle_t event)
{
    if (event->current_data_offset == 0) {
        mqtt_rx_reset();
        s_rx_topic[0] = '\0';
        if (event->topic != NULL && event->topic_len > 0 && event->topic_len < (int)sizeof(s_rx_topic)) {
            memcpy(s_rx_topic, event->topic, (size_t)event->topic_len);
            s_rx_topic[event->topic_len] = '\0';
        }

        if (event->total_data_len <= 0) {
            return;
        }
        size_t required = (size_t)event->total_data_len + 1U;
        if (required > PRINTER_MQTT_RX_MAX + 1U) {
            ESP_LOGW(TAG, "MQTT payload too large bytes=%u", (unsigned)event->total_data_len);
            return;
        }
        s_rx_dynamic = heap_caps_malloc(required, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (s_rx_dynamic == NULL) {
            s_rx_dynamic = malloc(required);
        }
        if (s_rx_dynamic == NULL) {
            ESP_LOGW(TAG, "MQTT payload allocation failed bytes=%u", (unsigned)required);
            return;
        }
        s_rx_dynamic_cap = required;
    }

    if (event->total_data_len <= 0 || event->current_data_offset < 0 || event->data_len < 0 ||
        event->current_data_offset + event->data_len > event->total_data_len || s_rx_dynamic == NULL) {
        return;
    }

    if ((size_t)event->total_data_len + 1U > s_rx_dynamic_cap) {
        return;
    }

    memcpy(s_rx_dynamic + event->current_data_offset, event->data, (size_t)event->data_len);
    if (event->current_data_offset + event->data_len == event->total_data_len) {
        s_rx_dynamic[event->total_data_len] = '\0';
        process_mqtt_message(s_rx_topic, s_rx_dynamic);
        mqtt_rx_reset();
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    (void)handler_args;
    (void)base;
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "printer MQTT connected");
        xEventGroupSetBits(s_events, PRINTER_EVENT_CONNECTED);
        xEventGroupClearBits(s_events, PRINTER_EVENT_REGISTERED | PRINTER_EVENT_ATTRIBUTES | PRINTER_EVENT_STATUS | PRINTER_EVENT_THUMBNAIL);
        state_set_attributes_loaded(false);
        s_subscribe_msg_id = esp_mqtt_client_subscribe(s_mqtt, s_topic_all, 0);
        break;
    case MQTT_EVENT_SUBSCRIBED:
        if (event->msg_id == s_subscribe_msg_id) {
            ESP_LOGI(TAG, "printer MQTT subscribed");
            publish_register();
        }
        break;
    case MQTT_EVENT_DISCONNECTED:
        if (state_service_requested()) {
            ESP_LOGW(TAG, "printer MQTT disconnected");
        }
        xEventGroupClearBits(s_events,
                             PRINTER_EVENT_CONNECTED | PRINTER_EVENT_REGISTERED |
                                 PRINTER_EVENT_ATTRIBUTES | PRINTER_EVENT_STATUS | PRINTER_EVENT_THUMBNAIL);
        state_set_attributes_loaded(false);
        printer_set_available(false);
        break;
    case MQTT_EVENT_DATA:
        mqtt_data_event(event);
        break;
    case MQTT_EVENT_ERROR:
        if (state_service_requested()) {
            ESP_LOGW(TAG, "printer MQTT error event");
        }
        break;
    default:
        break;
    }
}

static void mqtt_destroy(void)
{
    printer_set_available(false);
    if (s_events != NULL) {
        xEventGroupClearBits(s_events,
                             PRINTER_EVENT_CONNECTED | PRINTER_EVENT_REGISTERED |
                                 PRINTER_EVENT_ATTRIBUTES | PRINTER_EVENT_STATUS | PRINTER_EVENT_THUMBNAIL);
    }
    if (s_mqtt != NULL) {
        (void)esp_mqtt_client_stop(s_mqtt);
        (void)esp_mqtt_client_destroy(s_mqtt);
        s_mqtt = NULL;
    }
    mqtt_rx_reset();
    state_reset_requests();
    bool had_job = state_reset_job();
    if (had_job) {
        (void)printer_thumbnail_clear();
    }
}

static bool mqtt_start(void)
{
    ESP_LOGI(TAG, "printer MQTT start host=%s port=%u", PRINTER_HOST, (unsigned)PRINTER_MQTT_PORT);
    uint32_t r1 = esp_random();
    uint32_t r2 = esp_random();
    snprintf(s_client_id, sizeof(s_client_id), "1_PC_%08lx%08lx", (unsigned long)r1, (unsigned long)r2);
    snprintf(s_request_prefix, sizeof(s_request_prefix), "%s_req", s_client_id);
    snprintf(s_topic_request, sizeof(s_topic_request), "elegoo/%s/%s/api_request", s_serial, s_client_id);
    snprintf(s_topic_response, sizeof(s_topic_response), "elegoo/%s/%s/api_response", s_serial, s_client_id);
    snprintf(s_topic_register_response,
             sizeof(s_topic_register_response),
             "elegoo/%s/%s/register_response",
             s_serial,
             s_request_prefix);
    snprintf(s_topic_all, sizeof(s_topic_all), "elegoo/%s/#", s_serial);
    snprintf(s_mqtt_uri, sizeof(s_mqtt_uri), "mqtt://%s:%u", PRINTER_HOST, (unsigned)PRINTER_MQTT_PORT);

    esp_mqtt_client_config_t cfg = {
        .broker = {
            .address = {
                .uri = s_mqtt_uri,
            },
        },
        .credentials = {
            .username = "elegoo",
            .client_id = s_client_id,
            .authentication = {
                .password = PRINTER_ACCESS_CODE,
            },
        },
        .session = {
            .keepalive = 60,
        },
        .network = {
            .reconnect_timeout_ms = PRINTER_RETRY_INTERVAL_MS,
            .timeout_ms = PRINTER_MQTT_TIMEOUT_MS,
            .disable_auto_reconnect = false,
        },
        .buffer = {
            .size = 8192,
            .out_size = 2048,
        },
    };

    s_mqtt = esp_mqtt_client_init(&cfg);
    if (s_mqtt == NULL) {
        ESP_LOGE(TAG, "printer MQTT init failed");
        return false;
    }
    esp_err_t err = esp_mqtt_client_register_event(s_mqtt, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "printer MQTT event registration failed: %s", esp_err_to_name(err));
        mqtt_destroy();
        return false;
    }
    err = esp_mqtt_client_start(s_mqtt);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "printer MQTT client start failed: %s", esp_err_to_name(err));
        mqtt_destroy();
        return false;
    }
    ESP_LOGI(TAG, "printer MQTT client started");
    return true;
}

static bool wait_for_session(void)
{
    EventBits_t bits = xEventGroupWaitBits(s_events,
                                           PRINTER_EVENT_REGISTERED | PRINTER_EVENT_STOP,
                                           pdFALSE,
                                           pdFALSE,
                                           pdMS_TO_TICKS(PRINTER_MQTT_TIMEOUT_MS * 2));
    if ((bits & PRINTER_EVENT_STOP) != 0 || !state_service_requested()) {
        return false;
    }
    bits = xEventGroupGetBits(s_events);
    return (bits & (PRINTER_EVENT_CONNECTED | PRINTER_EVENT_REGISTERED)) ==
           (PRINTER_EVENT_CONNECTED | PRINTER_EVENT_REGISTERED);
}

static bool request_attributes(void)
{
    xEventGroupClearBits(s_events, PRINTER_EVENT_ATTRIBUTES);
    int request_id = next_request_id();
    state_set_pending_attributes(request_id);
    if (!publish_request(1001, request_id)) {
        return false;
    }
    EventBits_t bits = xEventGroupWaitBits(s_events,
                                           PRINTER_EVENT_ATTRIBUTES | PRINTER_EVENT_STOP,
                                           pdTRUE,
                                           pdFALSE,
                                           pdMS_TO_TICKS(PRINTER_MQTT_TIMEOUT_MS));
    if ((bits & PRINTER_EVENT_STOP) != 0 || !state_service_requested()) {
        return false;
    }
    return (bits & PRINTER_EVENT_ATTRIBUTES) != 0;
}

static bool request_status(void)
{
    xEventGroupClearBits(s_events, PRINTER_EVENT_STATUS);
    int request_id = next_request_id();
    state_set_pending_status(request_id);
    if (!publish_request(1002, request_id)) {
        return false;
    }
    EventBits_t bits = xEventGroupWaitBits(s_events,
                                           PRINTER_EVENT_STATUS | PRINTER_EVENT_STOP,
                                           pdTRUE,
                                           pdFALSE,
                                           pdMS_TO_TICKS(PRINTER_MQTT_TIMEOUT_MS));
    if ((bits & PRINTER_EVENT_STOP) != 0 || !state_service_requested()) {
        return false;
    }
    return (bits & PRINTER_EVENT_STATUS) != 0;
}

static printer_thumbnail_fetch_result_t request_thumbnail(const char *filename)
{
    xEventGroupClearBits(s_events, PRINTER_EVENT_THUMBNAIL);
    int request_id = next_request_id();
    state_set_pending_thumbnail(request_id, filename);
    if (!publish_thumbnail_request(request_id, filename)) {
        ESP_LOGW(TAG, "thumbnail method=1045 publish failed file=%s", filename);
        return THUMBNAIL_FETCH_RETRY;
    }

    EventBits_t bits = xEventGroupWaitBits(s_events,
                                           PRINTER_EVENT_THUMBNAIL | PRINTER_EVENT_STOP,
                                           pdTRUE,
                                           pdFALSE,
                                           pdMS_TO_TICKS(PRINTER_MQTT_TIMEOUT_MS));
    if ((bits & PRINTER_EVENT_STOP) != 0 || !state_service_requested()) {
        return THUMBNAIL_FETCH_RETRY;
    }
    if ((bits & PRINTER_EVENT_THUMBNAIL) == 0) {
        ESP_LOGW(TAG, "thumbnail method=1045 timeout file=%s", filename);
        return THUMBNAIL_FETCH_RETRY;
    }
    return state_get_thumbnail_request_result();
}

static void wait_or_notify(uint32_t delay_ms)
{
    if (s_events == NULL) {
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
        return;
    }
    (void)xEventGroupWaitBits(s_events,
                              PRINTER_EVENT_STOP | PRINTER_EVENT_WAKE,
                              pdTRUE,
                              pdFALSE,
                              pdMS_TO_TICKS(delay_ms));
}

static void printer_task(void *arg)
{
    (void)arg;
    int64_t last_ping_us = 0;
    bool config_warning_logged = false;

    ESP_LOGI(TAG, "printer service active");

    for (;;) {
        while (state_service_requested()) {
            if (!printer_configured()) {
                printer_set_available(false);
                if (!config_warning_logged) {
                    ESP_LOGW(TAG, "printer disabled: set PRINTER_HOST and PRINTER_ACCESS_CODE in printer_config.h");
                    config_warning_logged = true;
                }
                wait_or_notify(PRINTER_RETRY_INTERVAL_MS);
                continue;
            }
            config_warning_logged = false;

            if (!wifi_connected()) {
                mqtt_destroy();
                wait_or_notify(PRINTER_RETRY_INTERVAL_MS);
                continue;
            }

            if (s_serial[0] == '\0') {
                if (!fetch_serial_number()) {
                    printer_set_available(false);
                    wait_or_notify(PRINTER_RETRY_INTERVAL_MS);
                    continue;
                }
                ESP_LOGI(TAG, "printer bootstrap ok host=%s", PRINTER_HOST);
            }
            if (!state_service_requested()) {
                break;
            }

            if (s_mqtt == NULL && !mqtt_start()) {
                if (!state_service_requested()) {
                    break;
                }
                printer_set_available(false);
                s_serial[0] = '\0';
                wait_or_notify(PRINTER_RETRY_INTERVAL_MS);
                continue;
            }

            if (!wait_for_session()) {
                if (!state_service_requested()) {
                    break;
                }
                ESP_LOGW(TAG, "printer MQTT session unavailable");
                mqtt_destroy();
                s_serial[0] = '\0';
                wait_or_notify(PRINTER_RETRY_INTERVAL_MS);
                continue;
            }

            if (!state_attributes_loaded()) {
                if (!request_attributes()) {
                    if (!state_service_requested()) {
                        break;
                    }
                    ESP_LOGW(TAG, "printer attributes unavailable; continuing with status");
                    state_set_attributes_loaded(true);
                } else {
                    wait_or_notify(2000);
                    if (!state_service_requested()) {
                        break;
                    }
                }
            }

            int64_t now_us = esp_timer_get_time();
            if (last_ping_us == 0) {
                last_ping_us = now_us;
            } else if (now_us - last_ping_us >= (int64_t)PRINTER_APP_PING_INTERVAL_MS * 1000LL) {
                publish_app_ping();
                last_ping_us = now_us;
            }

            if (!request_status()) {
                if (!state_service_requested()) {
                    break;
                }
                ESP_LOGW(TAG, "printer status unavailable");
                mqtt_destroy();
                s_serial[0] = '\0';
                wait_or_notify(PRINTER_RETRY_INTERVAL_MS);
                continue;
            }

            refresh_printer_thumbnail();
            wait_or_notify(PRINTER_POLL_INTERVAL_MS);
        }

        mqtt_destroy();
        s_serial[0] = '\0';
        last_ping_us = 0;
        apply_unavailable_values();

        portENTER_CRITICAL(&s_state_mux);
        if (s_service_requested) {
            portEXIT_CRITICAL(&s_state_mux);
            if (s_events != NULL) {
                xEventGroupClearBits(s_events, PRINTER_EVENT_STOP);
            }
            continue;
        }
        s_task = NULL;
        portEXIT_CRITICAL(&s_state_mux);

        ESP_LOGI(TAG, "printer service stopped");
        vTaskDelete(NULL);
        return;
    }
}

esp_err_t printer_service_start(void)
{
    if (s_events == NULL) {
        s_events = xEventGroupCreate();
        if (s_events == NULL) {
            ESP_LOGE(TAG, "printer event group allocation failed");
            return ESP_ERR_NO_MEM;
        }
    }

    state_set_service_requested(true);
    xEventGroupClearBits(s_events,
                         PRINTER_EVENT_STOP | PRINTER_EVENT_WAKE | PRINTER_EVENT_AVAILABLE |
                             PRINTER_EVENT_CONNECTED | PRINTER_EVENT_REGISTERED |
                             PRINTER_EVENT_ATTRIBUTES | PRINTER_EVENT_STATUS | PRINTER_EVENT_THUMBNAIL);

    TaskHandle_t task = s_task;
    if (task != NULL) {
        xEventGroupSetBits(s_events, PRINTER_EVENT_WAKE);
        return ESP_OK;
    }

    BaseType_t ok = xTaskCreate(printer_task, "printer_service", 10240, NULL, tskIDLE_PRIORITY + 2, &s_task);
    if (ok != pdPASS) {
        s_task = NULL;
        state_set_service_requested(false);
        xEventGroupSetBits(s_events, PRINTER_EVENT_STOP);
        ESP_LOGE(TAG, "printer task allocation failed");
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

void printer_service_stop(void)
{
    state_set_service_requested(false);
    printer_set_available(false);
    if (s_events != NULL) {
        xEventGroupSetBits(s_events, PRINTER_EVENT_STOP | PRINTER_EVENT_WAKE);
    }
}

void printer_service_request_update(void)
{
    if (s_events != NULL && s_task != NULL && state_service_requested()) {
        xEventGroupSetBits(s_events, PRINTER_EVENT_WAKE);
    }
}
