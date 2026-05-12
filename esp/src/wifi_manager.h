#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_wifi_types.h"

#define PULSEMON_WIFI_STATUS_SSID_MAX_LEN 32

typedef void (*pulsemon_wifi_connected_cb_t)(void);

typedef struct {
    bool connected;
    bool ap_active;
    bool has_credentials;
    char ssid[PULSEMON_WIFI_STATUS_SSID_MAX_LEN + 1];
    char ip[16];
} pulsemon_wifi_status_t;

typedef struct {
    char ssid[PULSEMON_WIFI_STATUS_SSID_MAX_LEN + 1];
    int8_t rssi;
    wifi_auth_mode_t authmode;
} pulsemon_wifi_scan_result_t;

esp_err_t pulsemon_wifi_manager_init(pulsemon_wifi_connected_cb_t connected_cb);
esp_err_t pulsemon_wifi_manager_start(void);
esp_err_t pulsemon_wifi_manager_apply_credentials(const char *ssid, const char *password);
esp_err_t pulsemon_wifi_manager_clear_credentials(void);
esp_err_t pulsemon_wifi_manager_scan(pulsemon_wifi_scan_result_t *results, uint16_t max_results, uint16_t *out_count);
void pulsemon_wifi_manager_get_status(pulsemon_wifi_status_t *out);
const char *pulsemon_wifi_authmode_name(wifi_auth_mode_t authmode);

#endif
