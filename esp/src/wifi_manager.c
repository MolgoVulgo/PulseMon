#include "wifi_manager.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"

#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "wifi_config.h"
#include "wifi_credentials.h"

static const char *TAG = "wifi_manager";

static esp_netif_t *s_sta_netif;
static esp_netif_t *s_ap_netif;
static esp_event_handler_instance_t s_wifi_handler_any_id;
static esp_event_handler_instance_t s_wifi_handler_got_ip;
static SemaphoreHandle_t s_state_lock;
static SemaphoreHandle_t s_scan_lock;
static TimerHandle_t s_manual_config_timer;
static pulsemon_wifi_connected_cb_t s_connected_cb;
static pulsemon_wifi_config_mode_cb_t s_config_mode_cb;
static int s_retry_count;
static bool s_started;
static bool s_connected;
static bool s_ap_active;
static bool s_has_credentials;
static bool s_scan_active;
static bool s_manual_config_active;
static int64_t s_manual_config_deadline_us;
static char s_current_ssid[PULSEMON_WIFI_STATUS_SSID_MAX_LEN + 1];
static char s_current_ip[16];
static wifi_ap_record_t s_scan_records[PULSEMON_WIFI_SCAN_MAX_RESULTS];

static void state_lock(void)
{
    if (s_state_lock != NULL) {
        xSemaphoreTake(s_state_lock, portMAX_DELAY);
    }
}

static void state_unlock(void)
{
    if (s_state_lock != NULL) {
        xSemaphoreGive(s_state_lock);
    }
}

static void set_connected_state(bool connected, const char *ssid, const char *ip)
{
    state_lock();
    s_connected = connected;
    if (ssid != NULL) {
        snprintf(s_current_ssid, sizeof(s_current_ssid), "%s", ssid);
    } else if (!connected) {
        s_current_ssid[0] = '\0';
    }
    if (ip != NULL) {
        snprintf(s_current_ip, sizeof(s_current_ip), "%s", ip);
    } else if (!connected) {
        s_current_ip[0] = '\0';
    }
    state_unlock();
}

static void set_ap_active(bool active)
{
    bool changed;

    state_lock();
    changed = s_ap_active != active;
    s_ap_active = active;
    state_unlock();

    /*
     * esp_wifi_set_config(WIFI_IF_AP, ...) can briefly restart the soft-AP
     * after switching from STA to APSTA.  Preserve the manual-window timer
     * across that transient AP_STOP/AP_START cycle.  The timeout callback is
     * the owner of manual-window expiration and clears the state explicitly.
     */
    if (changed && s_config_mode_cb != NULL) {
        s_config_mode_cb(active);
    }
}

static esp_err_t configure_ap(void)
{
    wifi_config_t ap_config = {0};
    snprintf((char *)ap_config.ap.ssid, sizeof(ap_config.ap.ssid), "%s", PULSEMON_WIFI_AP_SSID);
    snprintf((char *)ap_config.ap.password, sizeof(ap_config.ap.password), "%s", PULSEMON_WIFI_AP_PASSWORD);
    ap_config.ap.ssid_len = strlen(PULSEMON_WIFI_AP_SSID);
    ap_config.ap.channel = 1;
    ap_config.ap.max_connection = 4;
    ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    ap_config.ap.pmf_cfg.required = false;

    if (strlen(PULSEMON_WIFI_AP_PASSWORD) == 0) {
        ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    esp_err_t err = esp_wifi_set_config(WIFI_IF_AP, &ap_config);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "config portal ap configured ssid=%s", PULSEMON_WIFI_AP_SSID);
    }
    return err;
}

static esp_err_t enable_config_ap(void)
{
    wifi_mode_t mode = WIFI_MODE_NULL;
    esp_err_t err = esp_wifi_get_mode(&mode);
    if (err != ESP_OK) {
        return err;
    }

    if (mode != WIFI_MODE_APSTA) {
        err = esp_wifi_set_mode(WIFI_MODE_APSTA);
        if (err != ESP_OK) {
            return err;
        }
    }
    return configure_ap();
}

static esp_err_t disable_config_ap_if_connected(void)
{
    state_lock();
    bool connected = s_connected;
    bool ap_active = s_ap_active;
    bool manual_config_active = s_manual_config_active;
    state_unlock();

    if (!connected || !ap_active || manual_config_active) {
        return ESP_OK;
    }

    return esp_wifi_set_mode(WIFI_MODE_STA);
}

static void manual_config_timeout_cb(TimerHandle_t timer)
{
    (void)timer;

    state_lock();
    bool should_disable = s_manual_config_active && s_connected && s_ap_active;
    s_manual_config_active = false;
    s_manual_config_deadline_us = 0;
    state_unlock();

    if (!should_disable) {
        return;
    }

    ESP_LOGI(TAG, "manual configuration window expired, disabling setup ap");
    esp_err_t err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "unable to disable setup ap after timeout: %s", esp_err_to_name(err));
    }
}

static esp_err_t configure_sta(const pulsemon_wifi_credentials_t *credentials)
{
    if (credentials == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    wifi_config_t sta_config = {0};
    size_t ssid_len = strnlen(credentials->ssid, sizeof(sta_config.sta.ssid));
    memcpy(sta_config.sta.ssid, credentials->ssid, ssid_len);
    snprintf((char *)sta_config.sta.password, sizeof(sta_config.sta.password), "%s", credentials->password);
    sta_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
    sta_config.sta.pmf_cfg.capable = true;
    sta_config.sta.pmf_cfg.required = false;

    esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &sta_config);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "sta configured ssid=%s", credentials->ssid);
    }
    return err;
}

static esp_err_t connect_sta_from_nvs(void)
{
    pulsemon_wifi_credentials_t credentials;
    esp_err_t err = pulsemon_wifi_credentials_load(&credentials);
    if (err != ESP_OK) {
        state_lock();
        s_has_credentials = false;
        state_unlock();
        ESP_LOGW(TAG, "no valid wifi credentials in nvs");
        return err;
    }

    state_lock();
    s_has_credentials = true;
    state_unlock();

    err = configure_sta(&credentials);
    if (err != ESP_OK) {
        return err;
    }

    s_retry_count = 0;
    set_connected_state(false, credentials.ssid, NULL);
    ESP_LOGI(TAG, "connecting sta ssid=%s", credentials.ssid);
    return esp_wifi_connect();
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    (void)arg;
    (void)event_data;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_START) {
        ESP_LOGI(TAG, "config portal ap active ssid=%s", PULSEMON_WIFI_AP_SSID);
        set_ap_active(true);
        return;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STOP) {
        ESP_LOGI(TAG, "config portal ap stopped");
        set_ap_active(false);
        return;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        if (pulsemon_wifi_credentials_present()) {
            esp_err_t err = connect_sta_from_nvs();
            if (err != ESP_OK) {
                ESP_LOGW(TAG, "sta connect setup failed: %s", esp_err_to_name(err));
                enable_config_ap();
            }
        } else {
            enable_config_ap();
        }
        return;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        set_connected_state(false, NULL, NULL);
        if (s_scan_active) {
            return;
        }
        if (pulsemon_wifi_credentials_present() && s_retry_count < PULSEMON_WIFI_MAX_RETRY) {
            s_retry_count++;
            ESP_LOGW(TAG, "wifi disconnected, retry %d/%d", s_retry_count, PULSEMON_WIFI_MAX_RETRY);
            esp_wifi_connect();
        } else {
            ESP_LOGW(TAG, "wifi unavailable, enabling config portal");
            enable_config_ap();
        }
        return;
    }

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        char ip[16];
        snprintf(ip, sizeof(ip), IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_count = 0;
        set_connected_state(true, NULL, ip);
        ESP_LOGI(TAG, "wifi connected ip=%s", ip);
        disable_config_ap_if_connected();
        if (s_connected_cb != NULL) {
            s_connected_cb();
        }
    }
}

esp_err_t pulsemon_wifi_manager_init(
    pulsemon_wifi_connected_cb_t connected_cb, pulsemon_wifi_config_mode_cb_t config_mode_cb)
{
    s_connected_cb = connected_cb;
    s_config_mode_cb = config_mode_cb;

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "nvs init failed (%s), erasing", esp_err_to_name(ret));
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ret = esp_netif_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        return ret;
    }

    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        return ret;
    }

    if (s_state_lock == NULL) {
        s_state_lock = xSemaphoreCreateMutex();
        if (s_state_lock == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }
    if (s_scan_lock == NULL) {
        s_scan_lock = xSemaphoreCreateMutex();
        if (s_scan_lock == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }
    if (s_manual_config_timer == NULL) {
        s_manual_config_timer = xTimerCreate(
            "config_window",
            pdMS_TO_TICKS(PULSEMON_WIFI_MANUAL_CONFIG_TIMEOUT_MS),
            pdFALSE,
            NULL,
            manual_config_timeout_cb);
        if (s_manual_config_timer == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }

    if (s_sta_netif == NULL) {
        s_sta_netif = esp_netif_create_default_wifi_sta();
        if (s_sta_netif == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }
    if (s_ap_netif == NULL) {
        s_ap_netif = esp_netif_create_default_wifi_ap();
        if (s_ap_netif == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        return ret;
    }

    ret = esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &s_wifi_handler_any_id);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        return ret;
    }

    ret = esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &s_wifi_handler_got_ip);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        return ret;
    }

    return ESP_OK;
}

esp_err_t pulsemon_wifi_manager_start(void)
{
    if (s_started) {
        return ESP_OK;
    }

    pulsemon_wifi_credentials_t credentials;
    esp_err_t load_err = pulsemon_wifi_credentials_load(&credentials);
    if (load_err == ESP_OK) {
        s_has_credentials = true;
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(configure_sta(&credentials));
    } else {
        s_has_credentials = false;
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
        ESP_ERROR_CHECK(configure_ap());
    }

    esp_err_t err = esp_wifi_start();
    if (err == ESP_OK) {
        s_started = true;
    }
    return err;
}

esp_err_t pulsemon_wifi_manager_open_config_mode(void)
{
    state_lock();
    bool started = s_started;
    s_manual_config_active = started;
    s_manual_config_deadline_us = 0;
    state_unlock();

    if (!started) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = enable_config_ap();
    if (err != ESP_OK) {
        state_lock();
        s_manual_config_active = false;
        s_manual_config_deadline_us = 0;
        state_unlock();
        return err;
    }

    if (s_manual_config_timer == NULL || xTimerReset(s_manual_config_timer, 0) != pdPASS) {
        state_lock();
        bool connected = s_connected;
        s_manual_config_active = false;
        s_manual_config_deadline_us = 0;
        state_unlock();
        if (connected) {
            esp_wifi_set_mode(WIFI_MODE_STA);
        }
        return ESP_FAIL;
    }

    state_lock();
    if (s_manual_config_active) {
        s_manual_config_deadline_us =
            esp_timer_get_time() + ((int64_t)PULSEMON_WIFI_MANUAL_CONFIG_TIMEOUT_MS * 1000LL);
    }
    state_unlock();

    ESP_LOGI(
        TAG,
        "manual configuration window opened for %u ms",
        (unsigned)PULSEMON_WIFI_MANUAL_CONFIG_TIMEOUT_MS);
    return ESP_OK;
}

esp_err_t pulsemon_wifi_manager_apply_credentials(const char *ssid, const char *password)
{
    esp_err_t err = pulsemon_wifi_credentials_save(ssid, password);
    if (err != ESP_OK) {
        return err;
    }

    pulsemon_wifi_credentials_t credentials;
    err = pulsemon_wifi_credentials_load(&credentials);
    if (err != ESP_OK) {
        return err;
    }

    state_lock();
    s_has_credentials = true;
    state_unlock();

    s_retry_count = 0;
    s_scan_active = false;
    set_connected_state(false, credentials.ssid, NULL);

    wifi_mode_t mode = WIFI_MODE_NULL;
    err = esp_wifi_get_mode(&mode);
    if (err != ESP_OK) {
        return err;
    }
    if (mode != WIFI_MODE_APSTA && mode != WIFI_MODE_STA) {
        err = esp_wifi_set_mode(WIFI_MODE_APSTA);
        if (err != ESP_OK) {
            return err;
        }
    }

    err = configure_sta(&credentials);
    if (err != ESP_OK) {
        return err;
    }

    esp_wifi_disconnect();
    ESP_LOGI(TAG, "applying saved wifi credentials ssid=%s", credentials.ssid);
    return esp_wifi_connect();
}

esp_err_t pulsemon_wifi_manager_clear_credentials(void)
{
    esp_err_t err = pulsemon_wifi_credentials_clear();
    if (err != ESP_OK) {
        return err;
    }

    state_lock();
    s_has_credentials = false;
    state_unlock();
    set_connected_state(false, NULL, NULL);
    esp_wifi_disconnect();
    return enable_config_ap();
}

static int compare_scan_results(const void *lhs, const void *rhs)
{
    const pulsemon_wifi_scan_result_t *a = (const pulsemon_wifi_scan_result_t *)lhs;
    const pulsemon_wifi_scan_result_t *b = (const pulsemon_wifi_scan_result_t *)rhs;
    return (int)b->rssi - (int)a->rssi;
}

esp_err_t pulsemon_wifi_manager_scan(pulsemon_wifi_scan_result_t *results, uint16_t max_results, uint16_t *out_count)
{
    if (results == NULL || out_count == NULL || max_results == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    *out_count = 0;
    if (s_scan_lock == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (max_results > PULSEMON_WIFI_SCAN_MAX_RESULTS) {
        max_results = PULSEMON_WIFI_SCAN_MAX_RESULTS;
    }

    xSemaphoreTake(s_scan_lock, portMAX_DELAY);
    s_scan_active = true;
    wifi_scan_config_t scan_config = {0};
    esp_err_t err = esp_wifi_scan_start(&scan_config, true);
    s_scan_active = false;
    if (err != ESP_OK) {
        xSemaphoreGive(s_scan_lock);
        return err;
    }

    uint16_t ap_count = 0;
    err = esp_wifi_scan_get_ap_num(&ap_count);
    if (err != ESP_OK || ap_count == 0) {
        xSemaphoreGive(s_scan_lock);
        return err;
    }

    uint16_t record_count = ap_count;
    if (record_count > PULSEMON_WIFI_SCAN_MAX_RESULTS) {
        record_count = PULSEMON_WIFI_SCAN_MAX_RESULTS;
    }

    err = esp_wifi_scan_get_ap_records(&record_count, s_scan_records);
    if (err != ESP_OK) {
        xSemaphoreGive(s_scan_lock);
        return err;
    }

    for (uint16_t i = 0; i < record_count && *out_count < max_results; i++) {
        if (s_scan_records[i].ssid[0] == '\0') {
            continue;
        }
        pulsemon_wifi_scan_result_t *result = &results[*out_count];
        memset(result, 0, sizeof(*result));
        snprintf(result->ssid, sizeof(result->ssid), "%s", (const char *)s_scan_records[i].ssid);
        result->rssi = s_scan_records[i].rssi;
        result->authmode = s_scan_records[i].authmode;
        (*out_count)++;
    }

    qsort(results, *out_count, sizeof(results[0]), compare_scan_results);
    xSemaphoreGive(s_scan_lock);
    return ESP_OK;
}

void pulsemon_wifi_manager_get_status(pulsemon_wifi_status_t *out)
{
    if (out == NULL) {
        return;
    }
    memset(out, 0, sizeof(*out));
    state_lock();
    out->connected = s_connected;
    out->ap_active = s_ap_active;
    out->has_credentials = s_has_credentials;
    out->manual_config_active = s_manual_config_active;
    if (s_manual_config_active && s_manual_config_deadline_us > 0) {
        int64_t remaining_us = s_manual_config_deadline_us - esp_timer_get_time();
        if (remaining_us > 0) {
            uint64_t remaining_ms = ((uint64_t)remaining_us + 999ULL) / 1000ULL;
            out->manual_config_remaining_ms = remaining_ms > UINT32_MAX ? UINT32_MAX : (uint32_t)remaining_ms;
        }
    }
    snprintf(out->ssid, sizeof(out->ssid), "%s", s_current_ssid);
    snprintf(out->ip, sizeof(out->ip), "%s", s_current_ip);
    state_unlock();
}

const char *pulsemon_wifi_authmode_name(wifi_auth_mode_t authmode)
{
    switch (authmode) {
    case WIFI_AUTH_OPEN:
        return "open";
    case WIFI_AUTH_WEP:
        return "wep";
    case WIFI_AUTH_WPA_PSK:
        return "wpa";
    case WIFI_AUTH_WPA2_PSK:
        return "wpa2";
    case WIFI_AUTH_WPA_WPA2_PSK:
        return "wpa_wpa2";
    case WIFI_AUTH_WPA2_ENTERPRISE:
        return "wpa2_enterprise";
    case WIFI_AUTH_WPA3_PSK:
        return "wpa3";
    case WIFI_AUTH_WPA2_WPA3_PSK:
        return "wpa2_wpa3";
    default:
        return "unknown";
    }
}
