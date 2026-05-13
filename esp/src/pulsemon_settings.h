#ifndef PULSEMON_SETTINGS_H
#define PULSEMON_SETTINGS_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#define PULSEMON_OPENWEATHER_KEY_MAX_LEN 96
#define PULSEMON_LANGUAGE_MAX_LEN 8

typedef struct {
    char openweather_key[PULSEMON_OPENWEATHER_KEY_MAX_LEN + 1];
    int16_t gmt_offset_min;
    uint32_t openweather_city_id;
    char language[PULSEMON_LANGUAGE_MAX_LEN + 1];
} pulsemon_settings_t;

void pulsemon_settings_defaults(pulsemon_settings_t *out);
esp_err_t pulsemon_settings_load(pulsemon_settings_t *out);
esp_err_t pulsemon_settings_save(const pulsemon_settings_t *settings);
esp_err_t pulsemon_settings_clear(void);
bool pulsemon_settings_validate(const pulsemon_settings_t *settings);

#endif
