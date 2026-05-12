#ifndef PULSEMON_WEATHER_ICONS_H
#define PULSEMON_WEATHER_ICONS_H

#include <stdint.h>

#include "esp_err.h"
#include "lvgl.h"

esp_err_t pulsemon_weather_icons_init(void);
esp_err_t pulsemon_weather_icons_log_index(const char *bin_name);
esp_err_t pulsemon_weather_icons_set_main(uint16_t code, uint8_t variant);
esp_err_t pulsemon_weather_icons_set_object(lv_obj_t *target, const char *bin_name, uint16_t code, uint8_t variant);

#endif
