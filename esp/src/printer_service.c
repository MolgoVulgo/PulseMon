#include "printer_service.h"

#include <stdbool.h>
#include <ctype.h>
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
#include "printer_settings.h"
#include "printer_thumbnail.h"
#include "printer_thumbnail_fetch.h"
#include "pulsemon_diag.h"
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
#define PRINTER_EVENT_FILE_DETAIL BIT8

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
#define PRINTER_FILE_DETAIL_RETRY_MS 30000
#define PRINTER_DISPLAY_TEXT_CAP 64

static const char *TAG = "printer";

static TaskHandle_t s_task;
static EventGroupHandle_t s_events;
static esp_mqtt_client_handle_t s_mqtt;
static esp_timer_handle_t s_presence_timer;

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
static int s_pending_file_detail_id = -1;
static char s_pending_file_detail_filename[PRINTER_FILENAME_CAP];
static bool s_file_detail_request_ok;
static printer_thumbnail_fetch_result_t s_thumbnail_request_result = THUMBNAIL_FETCH_RETRY;
static bool s_attributes_loaded;
static char s_job_filename[PRINTER_FILENAME_CAP];
static bool s_thumbnail_pending;
static bool s_thumbnail_clear_pending;
static bool s_screen_active;
static bool s_background_cycle_requested;
static bool s_task_starting;
static bool s_presence_timer_started;
static bool s_settings_reload_requested = true;
static bool s_settings_loaded;
static printer_settings_t s_settings;
static bool s_elapsed_valid;
static uint32_t s_elapsed_base_seconds;
static int64_t s_elapsed_base_us;
static uint32_t s_job_current_layer = UINT32_MAX;
static uint32_t s_job_total_layers;
static bool s_file_detail_needed;
static char s_file_detail_last_attempt_filename[PRINTER_FILENAME_CAP];
static int64_t s_file_detail_last_attempt_us;
static char s_thumbnail_last_attempt_filename[PRINTER_FILENAME_CAP];
static int64_t s_thumbnail_last_attempt_us;

typedef struct {
    bool valid;
    char name[PRINTER_DISPLAY_TEXT_CAP];
    char ip[PRINTER_DISPLAY_TEXT_CAP];
    char filename[PRINTER_DISPLAY_TEXT_CAP];
    char start_time[PRINTER_DISPLAY_TEXT_CAP];
    char end_time[PRINTER_DISPLAY_TEXT_CAP];
    char elapsed[PRINTER_DISPLAY_TEXT_CAP];
    char remaining[PRINTER_DISPLAY_TEXT_CAP];
    char layer[PRINTER_DISPLAY_TEXT_CAP];
    int32_t progress;
} printer_display_cache_t;

static printer_display_cache_t *s_display_cache;
static portMUX_TYPE s_state_mux = portMUX_INITIALIZER_UNLOCKED;

static void format_duration(uint32_t seconds, char *out, size_t out_len);
static bool state_get_elapsed_seconds(uint32_t *seconds);

static bool ensure_display_cache(void)
{
    if (s_display_cache != NULL) {
        return true;
    }

    printer_display_cache_t *cache = heap_caps_malloc(
        sizeof(*cache), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (cache == NULL) {
        ESP_LOGW(TAG, "printer display cache PSRAM allocation failed bytes=%u",
                 (unsigned)sizeof(*cache));
        return false;
    }
    memset(cache, 0, sizeof(*cache));
    s_display_cache = cache;
    ESP_LOGI(TAG, "printer display cache allocated in PSRAM bytes=%u",
             (unsigned)sizeof(*cache));
    return true;
}

static void display_cache_clear(void)
{
    if (s_display_cache == NULL) {
        return;
    }
    portENTER_CRITICAL(&s_state_mux);
    memset(s_display_cache, 0, sizeof(*s_display_cache));
    portEXIT_CRITICAL(&s_state_mux);
}

static void display_cache_update_name(const char *name)
{
    if (s_display_cache == NULL || name == NULL || name[0] == '\0') {
        return;
    }

    char cached_name[PRINTER_DISPLAY_TEXT_CAP] = {0};
    snprintf(cached_name, sizeof(cached_name), "%s", name);
    portENTER_CRITICAL(&s_state_mux);
    memcpy(s_display_cache->name, cached_name, sizeof(cached_name));
    portEXIT_CRITICAL(&s_state_mux);
}

static void display_cache_update_status(const char *ip,
                                        const char *filename,
                                        const char *start_time,
                                        const char *end_time,
                                        const char *elapsed,
                                        const char *remaining,
                                        const char *layer,
                                        int32_t progress)
{
    if (s_display_cache == NULL || filename == NULL || filename[0] == '\0') {
        display_cache_clear();
        return;
    }

    printer_display_cache_t next = {0};
    snprintf(next.ip, sizeof(next.ip), "%s", ip != NULL ? ip : "");
    snprintf(next.filename, sizeof(next.filename), "%s", filename);
    snprintf(next.start_time, sizeof(next.start_time), "%s", start_time != NULL ? start_time : "--:--");
    snprintf(next.end_time, sizeof(next.end_time), "%s", end_time != NULL ? end_time : "--:--");
    snprintf(next.elapsed, sizeof(next.elapsed), "%s", elapsed != NULL ? elapsed : "00:00:00");
    snprintf(next.remaining, sizeof(next.remaining), "%s", remaining != NULL ? remaining : "00:00:00");
    snprintf(next.layer, sizeof(next.layer), "%s", layer != NULL ? layer : "--/--");
    next.progress = progress;
    next.valid = true;

    portENTER_CRITICAL(&s_state_mux);
    memcpy(next.name, s_display_cache->name, sizeof(next.name));
    memcpy(s_display_cache, &next, sizeof(next));
    portEXIT_CRITICAL(&s_state_mux);
}

static void display_cache_update_layer(const char *layer)
{
    if (s_display_cache == NULL || layer == NULL || layer[0] == '\0') {
        return;
    }

    char cached_layer[PRINTER_DISPLAY_TEXT_CAP] = {0};
    snprintf(cached_layer, sizeof(cached_layer), "%s", layer);
    portENTER_CRITICAL(&s_state_mux);
    if (s_display_cache->valid) {
        memcpy(s_display_cache->layer, cached_layer, sizeof(cached_layer));
    }
    portEXIT_CRITICAL(&s_state_mux);
}

static bool display_cache_copy(printer_display_cache_t *out)
{
    if (out == NULL || s_display_cache == NULL) {
        return false;
    }
    portENTER_CRITICAL(&s_state_mux);
    memcpy(out, s_display_cache, sizeof(*out));
    portEXIT_CRITICAL(&s_state_mux);
    return out->valid;
}


static void state_reset_requests(void)
{
    portENTER_CRITICAL(&s_state_mux);
    s_pending_attributes_id = -1;
    s_pending_status_id = -1;
    s_pending_thumbnail_id = -1;
    s_pending_thumbnail_filename[0] = '\0';
    s_pending_file_detail_id = -1;
    s_pending_file_detail_filename[0] = '\0';
    s_file_detail_request_ok = false;
    s_thumbnail_request_result = THUMBNAIL_FETCH_RETRY;
    s_attributes_loaded = false;
    portEXIT_CRITICAL(&s_state_mux);
}

static bool state_task_should_run(void)
{
    portENTER_CRITICAL(&s_state_mux);
    bool requested = s_screen_active || s_background_cycle_requested;
    portEXIT_CRITICAL(&s_state_mux);
    return requested;
}

static bool state_screen_active(void)
{
    portENTER_CRITICAL(&s_state_mux);
    bool active = s_screen_active;
    portEXIT_CRITICAL(&s_state_mux);
    return active;
}

static void state_set_screen_active(bool active)
{
    portENTER_CRITICAL(&s_state_mux);
    s_screen_active = active;
    if (active) {
        s_background_cycle_requested = false;
    }
    portEXIT_CRITICAL(&s_state_mux);
}

static void state_request_background_cycle(void)
{
    portENTER_CRITICAL(&s_state_mux);
    if (!s_screen_active) {
        s_background_cycle_requested = true;
    }
    portEXIT_CRITICAL(&s_state_mux);
}

static void state_finish_background_cycle(void)
{
    portENTER_CRITICAL(&s_state_mux);
    if (!s_screen_active) {
        s_background_cycle_requested = false;
    }
    portEXIT_CRITICAL(&s_state_mux);
}

static void state_request_settings_reload(void)
{
    portENTER_CRITICAL(&s_state_mux);
    s_settings_reload_requested = true;
    portEXIT_CRITICAL(&s_state_mux);
}

static bool state_take_settings_reload(void)
{
    portENTER_CRITICAL(&s_state_mux);
    bool requested = s_settings_reload_requested;
    s_settings_reload_requested = false;
    portEXIT_CRITICAL(&s_state_mux);
    return requested;
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


static void state_set_pending_file_detail(int request_id, const char *filename)
{
    portENTER_CRITICAL(&s_state_mux);
    s_pending_file_detail_id = request_id;
    snprintf(s_pending_file_detail_filename,
             sizeof(s_pending_file_detail_filename),
             "%s",
             filename != NULL ? filename : "");
    s_file_detail_request_ok = false;
    portEXIT_CRITICAL(&s_state_mux);
}

static int state_get_pending_file_detail(char *filename, size_t filename_len)
{
    portENTER_CRITICAL(&s_state_mux);
    int request_id = s_pending_file_detail_id;
    if (filename != NULL && filename_len > 0) {
        snprintf(filename, filename_len, "%s", s_pending_file_detail_filename);
    }
    portEXIT_CRITICAL(&s_state_mux);
    return request_id;
}

static void state_complete_file_detail(bool ok, uint32_t total_layers)
{
    portENTER_CRITICAL(&s_state_mux);
    bool same_job = s_pending_file_detail_filename[0] != '\0' &&
                    strncmp(s_job_filename,
                            s_pending_file_detail_filename,
                            sizeof(s_job_filename)) == 0;
    s_file_detail_request_ok = ok && same_job && total_layers > 0;
    if (s_file_detail_request_ok) {
        s_job_total_layers = total_layers;
        s_file_detail_needed = false;
    }
    s_pending_file_detail_id = -1;
    s_pending_file_detail_filename[0] = '\0';
    portEXIT_CRITICAL(&s_state_mux);
}

static void state_cancel_file_detail_request(void)
{
    portENTER_CRITICAL(&s_state_mux);
    s_pending_file_detail_id = -1;
    s_pending_file_detail_filename[0] = '\0';
    s_file_detail_request_ok = false;
    portEXIT_CRITICAL(&s_state_mux);
}

static bool state_file_detail_request_ok(void)
{
    portENTER_CRITICAL(&s_state_mux);
    bool ok = s_file_detail_request_ok;
    portEXIT_CRITICAL(&s_state_mux);
    return ok;
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
        s_job_current_layer = UINT32_MAX;
        s_job_total_layers = 0;
        s_file_detail_needed = next[0] != '\0';
        s_pending_file_detail_id = -1;
        s_pending_file_detail_filename[0] = '\0';
        s_file_detail_request_ok = false;
        s_file_detail_last_attempt_filename[0] = '\0';
        s_file_detail_last_attempt_us = 0;
    }
    portEXIT_CRITICAL(&s_state_mux);

    if (changed) {
        ESP_LOGI(TAG,
                 "printer job filename changed file=%s thumbnail_pending=%d",
                 next[0] != '\0' ? next : "<none>",
                 next[0] != '\0' ? 1 : 0);
    }
}

static void state_update_layer_status(uint32_t current_layer, uint32_t total_layers)
{
    portENTER_CRITICAL(&s_state_mux);
    if (s_job_filename[0] != '\0') {
        if (current_layer != UINT32_MAX) {
            s_job_current_layer = current_layer;
        }
        if (total_layers > 0) {
            s_job_total_layers = total_layers;
            s_file_detail_needed = false;
        } else if (s_job_total_layers == 0) {
            s_file_detail_needed = true;
        }
    }
    portEXIT_CRITICAL(&s_state_mux);
}

static void state_format_layer(char *out, size_t out_len)
{
    if (out == NULL || out_len == 0) {
        return;
    }

    portENTER_CRITICAL(&s_state_mux);
    uint32_t current_layer = s_job_current_layer;
    uint32_t total_layers = s_job_total_layers;
    bool has_job = s_job_filename[0] != '\0';
    portEXIT_CRITICAL(&s_state_mux);

    if (!has_job || current_layer == UINT32_MAX) {
        snprintf(out, out_len, "--/--");
    } else if (total_layers > 0) {
        snprintf(out, out_len, "%lu/%lu",
                 (unsigned long)current_layer,
                 (unsigned long)total_layers);
    } else {
        snprintf(out, out_len, "%lu/--", (unsigned long)current_layer);
    }
}

static bool state_claim_file_detail_attempt(char *filename, size_t filename_len)
{
    if (filename == NULL || filename_len == 0) {
        return false;
    }

    int64_t now_us = esp_timer_get_time();
    portENTER_CRITICAL(&s_state_mux);
    bool ready = s_job_filename[0] != '\0' && s_file_detail_needed && s_job_total_layers == 0;
    bool same_attempt = strncmp(s_file_detail_last_attempt_filename,
                                s_job_filename,
                                sizeof(s_job_filename)) == 0;
    bool throttled = same_attempt && s_file_detail_last_attempt_us != 0 &&
                     now_us - s_file_detail_last_attempt_us <
                         (int64_t)PRINTER_FILE_DETAIL_RETRY_MS * 1000LL;
    if (ready && !throttled) {
        snprintf(filename, filename_len, "%s", s_job_filename);
        snprintf(s_file_detail_last_attempt_filename,
                 sizeof(s_file_detail_last_attempt_filename),
                 "%s",
                 s_job_filename);
        s_file_detail_last_attempt_us = now_us;
    }
    portEXIT_CRITICAL(&s_state_mux);
    return ready && !throttled;
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
    s_elapsed_valid = false;
    s_elapsed_base_seconds = 0;
    s_elapsed_base_us = 0;
    s_job_current_layer = UINT32_MAX;
    s_job_total_layers = 0;
    s_file_detail_needed = false;
    s_pending_file_detail_id = -1;
    s_pending_file_detail_filename[0] = '\0';
    s_file_detail_request_ok = false;
    s_file_detail_last_attempt_filename[0] = '\0';
    s_file_detail_last_attempt_us = 0;
    portEXIT_CRITICAL(&s_state_mux);
    return had_job;
}

static void state_set_elapsed_correction(bool valid, uint32_t seconds)
{
    int64_t now_us = esp_timer_get_time();
    portENTER_CRITICAL(&s_state_mux);
    s_elapsed_valid = valid;
    s_elapsed_base_seconds = valid ? seconds : 0;
    s_elapsed_base_us = valid ? now_us : 0;
    portEXIT_CRITICAL(&s_state_mux);
}

static bool state_get_elapsed_seconds(uint32_t *seconds)
{
    if (seconds == NULL) {
        return false;
    }

    portENTER_CRITICAL(&s_state_mux);
    bool valid = s_elapsed_valid;
    uint32_t base_seconds = s_elapsed_base_seconds;
    int64_t base_us = s_elapsed_base_us;
    portEXIT_CRITICAL(&s_state_mux);

    if (!valid || base_us <= 0) {
        return false;
    }

    int64_t now_us = esp_timer_get_time();
    uint64_t delta_seconds = now_us > base_us ? (uint64_t)(now_us - base_us) / 1000000ULL : 0ULL;
    uint64_t current = (uint64_t)base_seconds + delta_seconds;
    *seconds = current > UINT32_MAX ? UINT32_MAX : (uint32_t)current;
    return true;
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

bool printer_service_has_cached_display(void)
{
    printer_display_cache_t cache = {0};
    return display_cache_copy(&cache);
}

bool printer_service_restore_cached_display(void)
{
    printer_display_cache_t cache = {0};
    if (!display_cache_copy(&cache)) {
        return false;
    }

    set_var_name_printer(cache.name[0] != '\0' ? cache.name : "--");
    set_var_printer_ip(cache.ip[0] != '\0' ? cache.ip : "--");
    set_var_print_file_name(cache.filename);
    set_var_print_time_start(cache.start_time);
    set_var_print_time_end(cache.end_time);
    char elapsed_buf[32] = {0};
    const char *elapsed_value = cache.elapsed;
    uint32_t elapsed_seconds = 0;
    if (state_get_elapsed_seconds(&elapsed_seconds)) {
        format_duration(elapsed_seconds, elapsed_buf, sizeof(elapsed_buf));
        elapsed_value = elapsed_buf;
    }
    set_var_print_time_elapsed(elapsed_value);
    set_var_print_time_remaining(cache.remaining);
    set_var_print_layer(cache.layer[0] != '\0' ? cache.layer : "--/--");
    set_var_print_bar(cache.progress);
    return true;
}

static bool load_runtime_settings(void)
{
    printer_settings_t settings;
    esp_err_t err = printer_settings_load(&settings);
    if (err != ESP_OK && err != ESP_ERR_INVALID_SIZE) {
        ESP_LOGW(TAG, "printer settings load failed: %s", esp_err_to_name(err));
        return false;
    }
    if (err == ESP_ERR_INVALID_SIZE) {
        ESP_LOGW(TAG, "invalid printer settings ignored");
    }
    s_settings = settings;
    s_settings_loaded = true;
    return true;
}

static bool printer_configured(void)
{
    return s_settings.host[0] != '\0' && s_settings.access_code[0] != '\0';
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

static bool has_gcode_suffix(const char *filename)
{
    static const char suffix[] = ".gcode";
    if (filename == NULL) {
        return false;
    }

    size_t filename_len = strlen(filename);
    size_t suffix_len = sizeof(suffix) - 1U;
    if (filename_len < suffix_len) {
        return false;
    }

    const char *tail = filename + filename_len - suffix_len;
    for (size_t i = 0; i < suffix_len; ++i) {
        if (tolower((unsigned char)tail[i]) != suffix[i]) {
            return false;
        }
    }
    return true;
}

static bool extension_matches_ci(const char *text, const char *extension)
{
    size_t i = 0;
    for (; extension[i] != '\0'; ++i) {
        if (text[i] == '\0' || tolower((unsigned char)text[i]) != extension[i]) {
            return false;
        }
    }
    return text[i] == '\0' || !isalnum((unsigned char)text[i]);
}

static size_t find_source_extension_pos(const char *filename)
{
    static const char *extensions[] = {".stl", ".obj", ".3mf"};
    if (filename == NULL) {
        return SIZE_MAX;
    }

    size_t filename_len = strlen(filename);
    for (size_t pos = 0; pos < filename_len; ++pos) {
        if (filename[pos] != '.') {
            continue;
        }
        for (size_t i = 0; i < sizeof(extensions) / sizeof(extensions[0]); ++i) {
            if (extension_matches_ci(filename + pos, extensions[i])) {
                return pos;
            }
        }
    }
    return SIZE_MAX;
}

static void format_display_filename(const char *filename, char *out, size_t out_len)
{
    if (out == NULL || out_len == 0) {
        return;
    }
    out[0] = '\0';
    if (filename == NULL || filename[0] == '\0') {
        return;
    }

    size_t source_len = strlen(filename);
    size_t source_extension_pos = find_source_extension_pos(filename);
    if (source_extension_pos != SIZE_MAX && source_extension_pos > 0) {
        source_len = source_extension_pos;
    } else if (has_gcode_suffix(filename)) {
        source_len -= sizeof(".gcode") - 1U;
    }
    size_t copy_len = source_len < out_len - 1U ? source_len : out_len - 1U;
    memcpy(out, filename, copy_len);
    out[copy_len] = '\0';
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
    uint32_t current_layer = json_u32(print_status, "current_layer", UINT32_MAX);
    const char *layer_progress = json_string(print_status, "layerProgress");
    if (layer_progress == NULL || layer_progress[0] == '\0') {
        layer_progress = json_string(machine_status, "layerProgress");
    }
    if (layer_progress == NULL || layer_progress[0] == '\0') {
        layer_progress = json_string(result, "layerProgress");
    }
    uint32_t elapsed = json_u32(print_status, "print_duration", 0);
    uint32_t remaining = json_u32(print_status, "remaining_time_sec", 0);

    char display_filename[PRINTER_DISPLAY_TEXT_CAP] = {0};
    char elapsed_buf[32];
    char remaining_buf[32];
    char start_buf[16] = "--:--";
    char end_buf[16] = "--:--";
    char layer_buf[PRINTER_DISPLAY_TEXT_CAP] = "--/--";
    format_duration(elapsed, elapsed_buf, sizeof(elapsed_buf));
    format_duration(remaining, remaining_buf, sizeof(remaining_buf));

    bool job_present = filename != NULL && filename[0] != '\0';
    state_update_job_filename(job_present ? filename : NULL);
    state_set_elapsed_correction(job_present, elapsed);
    state_update_layer_status(job_present ? current_layer : UINT32_MAX, 0);
    if (job_present) {
        format_display_filename(filename, display_filename, sizeof(display_filename));
        if (current_layer != UINT32_MAX) {
            state_format_layer(layer_buf, sizeof(layer_buf));
        } else if (layer_progress != NULL && layer_progress[0] != '\0') {
            snprintf(layer_buf, sizeof(layer_buf), "%s", layer_progress);
        }
        time_t now = 0;
        time(&now);
        (void)format_clock(now - (time_t)elapsed, start_buf, sizeof(start_buf));
        (void)format_clock(now + (time_t)remaining, end_buf, sizeof(end_buf));
    }

    display_cache_update_status(s_settings.host,
                                job_present ? display_filename : NULL,
                                start_buf,
                                end_buf,
                                elapsed_buf,
                                remaining_buf,
                                layer_buf,
                                job_present ? progress : 0);

    if (state_screen_active() && bsp_display_lock(pdMS_TO_TICKS(100))) {
        set_var_printer_ip(s_settings.host);
        set_var_print_file_name(job_present ? display_filename : "--");
        set_var_print_time_start(job_present ? start_buf : "--:--");
        set_var_print_time_end(job_present ? end_buf : "--:--");
        set_var_print_time_elapsed(job_present ? elapsed_buf : "00:00:00");
        set_var_print_time_remaining(job_present ? remaining_buf : "00:00:00");
        set_var_print_layer(job_present ? layer_buf : "--/--");
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

    display_cache_update_name(name);

    if (state_screen_active() && bsp_display_lock(pdMS_TO_TICKS(100))) {
        set_var_name_printer(name);
        set_var_printer_ip(s_settings.host);
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
             s_settings.host,
             (unsigned)PRINTER_HTTP_PORT,
             s_settings.access_code);

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
        if (s_serial[0] != '\0' && strcmp(s_serial, sn) != 0) {
            state_set_attributes_loaded(false);
        }
        snprintf(s_serial, sizeof(s_serial), "%s", sn);
    }
    cJSON_Delete(root);
    return ok;
}

static bool publish_thumbnail_request(int request_id, const char *filename);
static bool publish_file_detail_request(int request_id, const char *filename);
static bool request_file_detail(const char *filename);
static printer_thumbnail_fetch_result_t request_thumbnail(const char *filename);

static void refresh_layer_display_from_state(void)
{
    char layer_buf[PRINTER_DISPLAY_TEXT_CAP] = "--/--";
    state_format_layer(layer_buf, sizeof(layer_buf));
    display_cache_update_layer(layer_buf);

    if (state_screen_active() && bsp_display_lock(pdMS_TO_TICKS(100))) {
        set_var_print_layer(layer_buf);
        bsp_display_unlock();
    }
}

static void refresh_printer_file_detail(void)
{
    char filename[PRINTER_FILENAME_CAP] = {0};
    if (!state_claim_file_detail_attempt(filename, sizeof(filename))) {
        return;
    }

    ESP_LOGI(TAG, "printer file detail MQTT request file=%s method=1046", filename);
    if (!request_file_detail(filename)) {
        ESP_LOGW(TAG,
                 "printer file detail MQTT unavailable file=%s retry_ms=%u",
                 filename,
                 (unsigned)PRINTER_FILE_DETAIL_RETRY_MS);
    }
}

static void refresh_printer_thumbnail(bool allow_fetch)
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
    if (!pending || filename[0] == '\0' || !allow_fetch) {
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

    char pending_file_detail_filename[PRINTER_FILENAME_CAP] = {0};
    int pending_file_detail_id = state_get_pending_file_detail(pending_file_detail_filename,
                                                                sizeof(pending_file_detail_filename));
    if (request_id == pending_file_detail_id) {
        uint32_t total_layers = 0;
        bool ok = false;
        if (error_code != 0) {
            ESP_LOGW(TAG,
                     "file detail method=1046 request id=%d error_code=%d file=%s",
                     request_id,
                     error_code,
                     pending_file_detail_filename[0] != '\0' ? pending_file_detail_filename : "<unknown>");
        } else if (!state_thumbnail_filename_is_current(pending_file_detail_filename)) {
            ESP_LOGI(TAG,
                     "file detail method=1046 stale response ignored file=%s",
                     pending_file_detail_filename[0] != '\0' ? pending_file_detail_filename : "<unknown>");
        } else {
            total_layers = json_u32(result, "layer", 0);
            if (total_layers == 0) {
                total_layers = json_u32(result, "TotalLayers", 0);
            }
            if (total_layers == 0) {
                total_layers = json_u32(result, "total_layer", 0);
            }
            ok = total_layers > 0;
            if (ok) {
                ESP_LOGI(TAG,
                         "printer file detail method=1046 file=%s total_layers=%lu",
                         pending_file_detail_filename,
                         (unsigned long)total_layers);
            } else {
                ESP_LOGW(TAG,
                         "file detail method=1046 response has no valid layer file=%s",
                         pending_file_detail_filename[0] != '\0' ? pending_file_detail_filename : "<unknown>");
            }
        }
        state_complete_file_detail(ok, total_layers);
        if (ok) {
            refresh_layer_display_from_state();
        }
        xEventGroupSetBits(s_events, PRINTER_EVENT_FILE_DETAIL);
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

static bool publish_file_detail_request(int request_id, const char *filename)
{
    if (filename == NULL || filename[0] == '\0') {
        return false;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON *params = cJSON_CreateObject();
    if (root == NULL || params == NULL ||
        !cJSON_AddNumberToObject(root, "id", request_id) ||
        !cJSON_AddNumberToObject(root, "method", 1046) ||
        !cJSON_AddItemToObject(root, "params", params) ||
        !cJSON_AddStringToObject(params, "storage_media", "local") ||
        !cJSON_AddStringToObject(params, "filename", filename)) {
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
        xEventGroupClearBits(s_events, PRINTER_EVENT_REGISTERED | PRINTER_EVENT_ATTRIBUTES | PRINTER_EVENT_STATUS | PRINTER_EVENT_THUMBNAIL |
                             PRINTER_EVENT_FILE_DETAIL);
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
        if (state_task_should_run()) {
            ESP_LOGW(TAG, "printer MQTT disconnected");
        }
        xEventGroupClearBits(s_events,
                             PRINTER_EVENT_CONNECTED | PRINTER_EVENT_REGISTERED |
                                 PRINTER_EVENT_ATTRIBUTES | PRINTER_EVENT_STATUS | PRINTER_EVENT_THUMBNAIL |
                             PRINTER_EVENT_FILE_DETAIL);
        state_set_attributes_loaded(false);
        break;
    case MQTT_EVENT_DATA:
        mqtt_data_event(event);
        break;
    case MQTT_EVENT_ERROR:
        if (state_task_should_run()) {
            ESP_LOGW(TAG, "printer MQTT error event");
        }
        break;
    default:
        break;
    }
}

static void mqtt_destroy(void)
{
    if (s_events != NULL) {
        xEventGroupClearBits(s_events,
                             PRINTER_EVENT_CONNECTED | PRINTER_EVENT_REGISTERED |
                                 PRINTER_EVENT_ATTRIBUTES | PRINTER_EVENT_STATUS | PRINTER_EVENT_THUMBNAIL |
                             PRINTER_EVENT_FILE_DETAIL);
    }
    if (s_mqtt != NULL) {
        (void)esp_mqtt_client_stop(s_mqtt);
        (void)esp_mqtt_client_destroy(s_mqtt);
        s_mqtt = NULL;
    }
    mqtt_rx_reset();
    state_reset_requests();
}

static bool mqtt_start(void)
{
    ESP_LOGI(TAG, "printer MQTT start host=%s port=%u", s_settings.host, (unsigned)PRINTER_MQTT_PORT);
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
    snprintf(s_mqtt_uri, sizeof(s_mqtt_uri), "mqtt://%s:%u", s_settings.host, (unsigned)PRINTER_MQTT_PORT);

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
                .password = s_settings.access_code,
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
                                           pdMS_TO_TICKS(PRINTER_MQTT_REGISTER_TIMEOUT_MS));
    if ((bits & PRINTER_EVENT_STOP) != 0 || !state_task_should_run()) {
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
    if ((bits & PRINTER_EVENT_STOP) != 0 || !state_task_should_run()) {
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
    if ((bits & PRINTER_EVENT_STOP) != 0 || !state_task_should_run()) {
        return false;
    }
    return (bits & PRINTER_EVENT_STATUS) != 0;
}

static bool request_file_detail(const char *filename)
{
    xEventGroupClearBits(s_events, PRINTER_EVENT_FILE_DETAIL);
    int request_id = next_request_id();
    state_set_pending_file_detail(request_id, filename);
    if (!publish_file_detail_request(request_id, filename)) {
        state_cancel_file_detail_request();
        ESP_LOGW(TAG, "file detail method=1046 publish failed file=%s", filename);
        return false;
    }

    EventBits_t bits = xEventGroupWaitBits(s_events,
                                           PRINTER_EVENT_FILE_DETAIL | PRINTER_EVENT_STOP,
                                           pdTRUE,
                                           pdFALSE,
                                           pdMS_TO_TICKS(PRINTER_MQTT_TIMEOUT_MS));
    if ((bits & PRINTER_EVENT_STOP) != 0 || !state_task_should_run()) {
        state_cancel_file_detail_request();
        return false;
    }
    if ((bits & PRINTER_EVENT_FILE_DETAIL) == 0) {
        state_cancel_file_detail_request();
        ESP_LOGW(TAG, "file detail method=1046 timeout file=%s", filename);
        return false;
    }
    return state_file_detail_request_ok();
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
    if ((bits & PRINTER_EVENT_STOP) != 0 || !state_task_should_run()) {
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

static void update_elapsed_display(void)
{
    if (!state_screen_active()) {
        return;
    }

    uint32_t elapsed_seconds = 0;
    if (!state_get_elapsed_seconds(&elapsed_seconds)) {
        return;
    }

    char elapsed_buf[32];
    format_duration(elapsed_seconds, elapsed_buf, sizeof(elapsed_buf));
    if (bsp_display_lock(pdMS_TO_TICKS(100))) {
        set_var_print_time_elapsed(elapsed_buf);
        bsp_display_unlock();
    }
}

static void wait_poll_interval_with_elapsed(void)
{
    int64_t deadline_us = esp_timer_get_time() + (int64_t)PRINTER_POLL_INTERVAL_MS * 1000LL;

    for (;;) {
        if (!state_task_should_run()) {
            return;
        }

        int64_t remaining_us = deadline_us - esp_timer_get_time();
        if (remaining_us <= 0) {
            return;
        }

        uint32_t wait_ms = (uint32_t)((remaining_us + 999LL) / 1000LL);
        if (wait_ms > 1000U) {
            wait_ms = 1000U;
        }

        if (s_events == NULL) {
            vTaskDelay(pdMS_TO_TICKS(wait_ms));
        } else {
            EventBits_t bits = xEventGroupWaitBits(s_events,
                                                   PRINTER_EVENT_STOP | PRINTER_EVENT_WAKE,
                                                   pdTRUE,
                                                   pdFALSE,
                                                   pdMS_TO_TICKS(wait_ms));
            if ((bits & (PRINTER_EVENT_STOP | PRINTER_EVENT_WAKE)) != 0) {
                return;
            }
        }

        update_elapsed_display();
    }
}

static void printer_task(void *arg);

static esp_err_t ensure_printer_task(void)
{
    bool create_task = false;

    portENTER_CRITICAL(&s_state_mux);
    if (s_task == NULL && !s_task_starting && (s_screen_active || s_background_cycle_requested)) {
        s_task_starting = true;
        create_task = true;
    }
    TaskHandle_t task = s_task;
    portEXIT_CRITICAL(&s_state_mux);

    if (!create_task) {
        if (task != NULL && s_events != NULL) {
            xEventGroupSetBits(s_events, PRINTER_EVENT_WAKE);
        }
        return ESP_OK;
    }

    BaseType_t ok = xTaskCreate(printer_task,
                                "printer_service",
                                PRINTER_TASK_STACK_SIZE,
                                NULL,
                                tskIDLE_PRIORITY + 2,
                                &s_task);

    portENTER_CRITICAL(&s_state_mux);
    s_task_starting = false;
    portEXIT_CRITICAL(&s_state_mux);

    if (ok != pdPASS) {
        portENTER_CRITICAL(&s_state_mux);
        s_task = NULL;
        portEXIT_CRITICAL(&s_state_mux);
        state_finish_background_cycle();
        ESP_LOGE(TAG, "printer task allocation failed");
        return ESP_ERR_NO_MEM;
    }

    bool retry_create = false;
    portENTER_CRITICAL(&s_state_mux);
    retry_create = s_task == NULL && (s_screen_active || s_background_cycle_requested);
    portEXIT_CRITICAL(&s_state_mux);
    if (retry_create) {
        return ensure_printer_task();
    }
    return ESP_OK;
}

static void printer_presence_timer_cb(void *arg)
{
    (void)arg;
    if (!wifi_connected() || state_screen_active()) {
        return;
    }
    state_request_background_cycle();
    if (s_events != NULL) {
        xEventGroupClearBits(s_events, PRINTER_EVENT_STOP);
    }
    (void)ensure_printer_task();
}

static void printer_task(void *arg)
{
    (void)arg;
    int64_t last_ping_us = 0;
    bool config_warning_logged = false;
    bool first_status_diag_logged = false;

    ESP_LOGI(TAG, "printer worker active");
    pulsemon_diag_heap("printer", "worker_start");

    for (;;) {
        while (state_task_should_run()) {
            bool screen_active = state_screen_active();
            bool reload_settings = state_take_settings_reload();
            if (!s_settings_loaded || reload_settings) {
                if (s_settings_loaded) {
                    mqtt_destroy();
                    s_serial[0] = '\0';
                    state_set_attributes_loaded(false);
                }
                if (!load_runtime_settings()) {
                    printer_set_available(false);
                    if (!screen_active) {
                        state_finish_background_cycle();
                        break;
                    }
                    wait_or_notify(PRINTER_RETRY_INTERVAL_MS);
                    continue;
                }
                config_warning_logged = false;
            }

            if (!printer_configured()) {
                printer_set_available(false);
                if (!config_warning_logged) {
                    ESP_LOGW(TAG, "printer disabled: configure printer host and access code in setup portal");
                    config_warning_logged = true;
                }
                if (!screen_active) {
                    state_finish_background_cycle();
                    break;
                }
                wait_or_notify(PRINTER_RETRY_INTERVAL_MS);
                continue;
            }
            config_warning_logged = false;

            if (!wifi_connected()) {
                mqtt_destroy();
                printer_set_available(false);
                if (!screen_active) {
                    state_finish_background_cycle();
                    break;
                }
                wait_or_notify(PRINTER_RETRY_INTERVAL_MS);
                continue;
            }

            if (s_mqtt == NULL) {
                if (!fetch_serial_number()) {
                    printer_set_available(false);
                    if (!screen_active) {
                        state_finish_background_cycle();
                        break;
                    }
                    wait_or_notify(PRINTER_RETRY_INTERVAL_MS);
                    continue;
                }
                ESP_LOGI(TAG, "printer presence probe ok host=%s", s_settings.host);
                printer_set_available(true);

                if (!state_task_should_run()) {
                    break;
                }

                if (!mqtt_start()) {
                    if (!state_screen_active()) {
                        state_finish_background_cycle();
                        break;
                    }
                    printer_set_available(false);
                    wait_or_notify(PRINTER_RETRY_INTERVAL_MS);
                    continue;
                }

                if (!wait_for_session()) {
                    if (!state_task_should_run()) {
                        break;
                    }
                    ESP_LOGW(TAG, "printer MQTT session unavailable");
                    mqtt_destroy();
                    if (!state_screen_active()) {
                        state_finish_background_cycle();
                        break;
                    }
                    printer_set_available(false);
                    wait_or_notify(PRINTER_RETRY_INTERVAL_MS);
                    continue;
                }

                if (!state_attributes_loaded()) {
                    if (!request_attributes()) {
                        if (!state_task_should_run()) {
                            break;
                        }
                        ESP_LOGW(TAG, "printer attributes unavailable; continuing with status");
                        state_set_attributes_loaded(true);
                    } else if (state_screen_active()) {
                        wait_or_notify(2000);
                        if (!state_task_should_run()) {
                            break;
                        }
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
                if (!state_task_should_run()) {
                    break;
                }
                ESP_LOGW(TAG, "printer status unavailable");
                mqtt_destroy();
                if (!state_screen_active()) {
                    state_finish_background_cycle();
                    break;
                }
                printer_set_available(false);
                wait_or_notify(PRINTER_RETRY_INTERVAL_MS);
                continue;
            }

            if (!first_status_diag_logged) {
                pulsemon_diag_heap("printer", "first_status");
                pulsemon_diag_stack("printer", "first_status");
                first_status_diag_logged = true;
            }

            screen_active = state_screen_active();
            refresh_printer_file_detail();
            refresh_printer_thumbnail(screen_active);
            if (!screen_active) {
                state_finish_background_cycle();
                break;
            }

            wait_poll_interval_with_elapsed();
        }

        mqtt_destroy();
        last_ping_us = 0;

        portENTER_CRITICAL(&s_state_mux);
        bool rerun = s_screen_active || s_background_cycle_requested;
        if (!rerun) {
            s_task = NULL;
        }
        portEXIT_CRITICAL(&s_state_mux);

        if (rerun) {
            if (s_events != NULL) {
                xEventGroupClearBits(s_events, PRINTER_EVENT_STOP);
            }
            continue;
        }

        pulsemon_diag_heap("printer", "worker_stop");
        pulsemon_diag_stack("printer", "worker_stop");
        ESP_LOGI(TAG, "printer worker stopped");
        vTaskDelete(NULL);
        return;
    }
}

esp_err_t printer_service_init(void)
{
    (void)ensure_display_cache();

    if (s_events == NULL) {
        s_events = xEventGroupCreate();
        if (s_events == NULL) {
            ESP_LOGE(TAG, "printer event group allocation failed");
            return ESP_ERR_NO_MEM;
        }
    }

    if (s_presence_timer == NULL) {
        const esp_timer_create_args_t timer_args = {
            .callback = printer_presence_timer_cb,
            .arg = NULL,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "printer_presence",
            .skip_unhandled_events = true,
        };
        esp_err_t err = esp_timer_create(&timer_args, &s_presence_timer);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "printer presence timer create failed: %s", esp_err_to_name(err));
            return err;
        }
    }

    if (!s_presence_timer_started) {
        esp_err_t err = esp_timer_start_periodic(
            s_presence_timer,
            (uint64_t)PRINTER_PRESENCE_INTERVAL_MS * 1000ULL);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "printer presence timer start failed: %s", esp_err_to_name(err));
            return err;
        }
        s_presence_timer_started = true;
    }

    return ESP_OK;
}

esp_err_t printer_service_start(void)
{
    esp_err_t err = printer_service_init();
    if (err != ESP_OK) {
        return err;
    }

    state_set_screen_active(true);
    xEventGroupClearBits(s_events, PRINTER_EVENT_STOP);
    xEventGroupSetBits(s_events, PRINTER_EVENT_WAKE);
    return ensure_printer_task();
}

void printer_service_stop(void)
{
    state_set_screen_active(false);
    state_finish_background_cycle();
    if (s_events != NULL) {
        xEventGroupSetBits(s_events, PRINTER_EVENT_STOP | PRINTER_EVENT_WAKE);
    }
}

void printer_service_reload_settings(void)
{
    display_cache_clear();
    bool had_job = state_reset_job();
    if (had_job) {
        (void)printer_thumbnail_clear();
    }
    state_request_settings_reload();
    state_set_attributes_loaded(false);
    printer_service_request_update();
}

void printer_service_request_update(void)
{
    esp_err_t err = printer_service_init();
    if (err != ESP_OK) {
        return;
    }

    if (!state_screen_active()) {
        state_request_background_cycle();
    }
    xEventGroupClearBits(s_events, PRINTER_EVENT_STOP);
    xEventGroupSetBits(s_events, PRINTER_EVENT_WAKE);
    (void)ensure_printer_task();
}
