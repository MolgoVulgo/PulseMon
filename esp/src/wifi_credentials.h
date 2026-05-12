#ifndef WIFI_CREDENTIALS_H
#define WIFI_CREDENTIALS_H

#include <stdbool.h>
#include "esp_err.h"

#define PULSEMON_WIFI_SSID_MAX_LEN 32
#define PULSEMON_WIFI_PASSWORD_MAX_LEN 64

typedef struct {
    char ssid[PULSEMON_WIFI_SSID_MAX_LEN + 1];
    char password[PULSEMON_WIFI_PASSWORD_MAX_LEN];
} pulsemon_wifi_credentials_t;

esp_err_t pulsemon_wifi_credentials_load(pulsemon_wifi_credentials_t *out);
esp_err_t pulsemon_wifi_credentials_save(const char *ssid, const char *password);
esp_err_t pulsemon_wifi_credentials_clear(void);
bool pulsemon_wifi_credentials_present(void);
bool pulsemon_wifi_credentials_validate(const char *ssid, const char *password);

#endif
