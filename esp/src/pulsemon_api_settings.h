#ifndef PULSEMON_API_SETTINGS_H
#define PULSEMON_API_SETTINGS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#define PULSEMON_API_HOST_MAX_LEN 95

typedef struct {
    char host[PULSEMON_API_HOST_MAX_LEN + 1];
    uint16_t port;
} pulsemon_api_settings_t;

void pulsemon_api_settings_defaults(pulsemon_api_settings_t *out);
bool pulsemon_api_settings_validate(const pulsemon_api_settings_t *settings);
esp_err_t pulsemon_api_settings_load(pulsemon_api_settings_t *out);
esp_err_t pulsemon_api_settings_save(const pulsemon_api_settings_t *settings);
esp_err_t pulsemon_api_settings_clear(void);
bool pulsemon_api_settings_build_base_url(
    const pulsemon_api_settings_t *settings,
    char *out,
    size_t out_len);

#endif
