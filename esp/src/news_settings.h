#ifndef NEWS_SETTINGS_H
#define NEWS_SETTINGS_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#define NEWS_PROVIDER_MAX_LEN 12
#define GNEWS_KEY_MAX_LEN 128
#define NEWS_CATEGORY_MAX_LEN 16
#define NEWS_LANG_MAX_LEN 8
#define NEWS_COUNTRY_MAX_LEN 8
#define NEWS_LAST_ERROR_MAX_LEN 24

typedef struct {
    char provider[NEWS_PROVIDER_MAX_LEN + 1];
    char gnews_key[GNEWS_KEY_MAX_LEN + 1];
    bool enabled;
    int16_t refresh_min;
    char category[NEWS_CATEGORY_MAX_LEN + 1];
    char lang[NEWS_LANG_MAX_LEN + 1];
    char country[NEWS_COUNTRY_MAX_LEN + 1];
    uint8_t max_items;
    uint8_t max_age_days;
    int64_t last_ok_ts;
    char last_error[NEWS_LAST_ERROR_MAX_LEN + 1];
} news_settings_t;

void news_settings_defaults(news_settings_t *out);
esp_err_t news_settings_load(news_settings_t *out);
esp_err_t news_settings_save(const news_settings_t *settings);
esp_err_t news_settings_clear(void);
bool news_settings_validate(const news_settings_t *settings);

#endif
