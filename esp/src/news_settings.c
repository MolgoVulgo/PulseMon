#include "news_settings.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "nvs.h"

#ifndef PULSEMON_NEWS_DEBUG
#define PULSEMON_NEWS_DEBUG 0
#endif

#if PULSEMON_NEWS_DEBUG
static const char *TAG = "news_settings";
#define NEWS_LOGI(fmt, ...) ESP_LOGI(TAG, fmt, ##__VA_ARGS__)
#define NEWS_LOGW(fmt, ...) ESP_LOGW(TAG, fmt, ##__VA_ARGS__)
#else
#define NEWS_LOGI(fmt, ...) ((void)0)
#define NEWS_LOGW(fmt, ...) ((void)0)
#endif

static const char *NVS_NAMESPACE = "news";
static const char *NVS_KEY_PROVIDER = "provider";
static const char *NVS_KEY_GNEWS_KEY = "gnews_key";
static const char *NVS_KEY_ENABLED = "enabled";
static const char *NVS_KEY_REFRESH_MIN = "refresh_min";
static const char *NVS_KEY_CATEGORY = "category";
static const char *NVS_KEY_LANG = "lang";
static const char *NVS_KEY_COUNTRY = "country";
static const char *NVS_KEY_MAX_ITEMS = "max_items";
static const char *NVS_KEY_MAX_AGE_DAYS = "max_age_days";
static const char *NVS_KEY_LAST_OK_TS = "last_ok_ts";
static const char *NVS_KEY_LAST_ERROR = "last_error";

static bool str_fits(const char *value, size_t cap)
{
    return value != NULL && strnlen(value, cap) < cap;
}

void news_settings_defaults(news_settings_t *out)
{
    if (out == NULL) {
        return;
    }
    memset(out, 0, sizeof(*out));
    snprintf(out->provider, sizeof(out->provider), "gnews");
    out->enabled = true;
    out->refresh_min = 30;
    snprintf(out->category, sizeof(out->category), "general");
    snprintf(out->lang, sizeof(out->lang), "fr");
    snprintf(out->country, sizeof(out->country), "fr");
    out->max_items = 5;
    out->max_age_days = 15;
    snprintf(out->last_error, sizeof(out->last_error), "none");
}

bool news_settings_validate(const news_settings_t *settings)
{
    if (settings == NULL) {
        return false;
    }
    if (strcmp(settings->provider, "gnews") != 0) {
        return false;
    }
    if (!str_fits(settings->gnews_key, sizeof(settings->gnews_key)) ||
        !str_fits(settings->category, sizeof(settings->category)) ||
        !str_fits(settings->lang, sizeof(settings->lang)) ||
        !str_fits(settings->country, sizeof(settings->country)) ||
        !str_fits(settings->last_error, sizeof(settings->last_error))) {
        return false;
    }
    if (settings->refresh_min < 15 || settings->refresh_min > 1440) {
        return false;
    }
    if (settings->max_items == 0 || settings->max_items > 5) {
        return false;
    }
    if (settings->max_age_days == 0 || settings->max_age_days > 15) {
        return false;
    }
    return true;
}

static esp_err_t nvs_get_str_optional(nvs_handle_t handle, const char *key, char *out, size_t out_len)
{
    size_t len = out_len;
    esp_err_t err = nvs_get_str(handle, key, out, &len);
    return err == ESP_ERR_NVS_NOT_FOUND ? ESP_OK : err;
}

esp_err_t news_settings_load(news_settings_t *out)
{
    if (out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    news_settings_defaults(out);

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        NEWS_LOGI("nvs namespace missing");
        return ESP_OK;
    }
    if (err != ESP_OK) {
        NEWS_LOGW("nvs open failed: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_get_str_optional(handle, NVS_KEY_PROVIDER, out->provider, sizeof(out->provider));
    if (err == ESP_OK) {
        err = nvs_get_str_optional(handle, NVS_KEY_GNEWS_KEY, out->gnews_key, sizeof(out->gnews_key));
    }
    uint8_t enabled = out->enabled ? 1 : 0;
    if (err == ESP_OK) {
        esp_err_t read_err = nvs_get_u8(handle, NVS_KEY_ENABLED, &enabled);
        if (read_err == ESP_OK) {
            out->enabled = enabled != 0;
        } else if (read_err != ESP_ERR_NVS_NOT_FOUND) {
            err = read_err;
        }
    }
    if (err == ESP_OK) {
        int16_t refresh_min = out->refresh_min;
        esp_err_t read_err = nvs_get_i16(handle, NVS_KEY_REFRESH_MIN, &refresh_min);
        if (read_err == ESP_OK) {
            out->refresh_min = refresh_min;
        } else if (read_err != ESP_ERR_NVS_NOT_FOUND) {
            err = read_err;
        }
    }
    if (err == ESP_OK) {
        err = nvs_get_str_optional(handle, NVS_KEY_CATEGORY, out->category, sizeof(out->category));
    }
    if (err == ESP_OK) {
        err = nvs_get_str_optional(handle, NVS_KEY_LANG, out->lang, sizeof(out->lang));
    }
    if (err == ESP_OK) {
        err = nvs_get_str_optional(handle, NVS_KEY_COUNTRY, out->country, sizeof(out->country));
    }
    if (err == ESP_OK) {
        uint8_t max_items = out->max_items;
        esp_err_t read_err = nvs_get_u8(handle, NVS_KEY_MAX_ITEMS, &max_items);
        if (read_err == ESP_OK) {
            out->max_items = max_items;
        } else if (read_err != ESP_ERR_NVS_NOT_FOUND) {
            err = read_err;
        }
    }
    if (err == ESP_OK) {
        uint8_t max_age_days = out->max_age_days;
        esp_err_t read_err = nvs_get_u8(handle, NVS_KEY_MAX_AGE_DAYS, &max_age_days);
        if (read_err == ESP_OK) {
            out->max_age_days = max_age_days;
        } else if (read_err != ESP_ERR_NVS_NOT_FOUND) {
            err = read_err;
        }
    }
    if (err == ESP_OK) {
        int64_t last_ok_ts = out->last_ok_ts;
        esp_err_t read_err = nvs_get_i64(handle, NVS_KEY_LAST_OK_TS, &last_ok_ts);
        if (read_err == ESP_OK) {
            out->last_ok_ts = last_ok_ts;
        } else if (read_err != ESP_ERR_NVS_NOT_FOUND) {
            err = read_err;
        }
    }
    if (err == ESP_OK) {
        err = nvs_get_str_optional(handle, NVS_KEY_LAST_ERROR, out->last_error, sizeof(out->last_error));
    }

    nvs_close(handle);
    if (err != ESP_OK) {
        NEWS_LOGW("settings load failed: %s", esp_err_to_name(err));
        return err;
    }
    if (!news_settings_validate(out)) {
        news_settings_defaults(out);
        NEWS_LOGW("settings invalid after load");
        return ESP_ERR_INVALID_SIZE;
    }
    NEWS_LOGI("settings load provider=%s enabled=%d key_set=%d key_len=%u refresh=%d category=%s lang=%s country=%s max=%u age_days=%u",
              out->provider,
              out->enabled,
              out->gnews_key[0] != '\0',
              (unsigned)strnlen(out->gnews_key, sizeof(out->gnews_key)),
              (int)out->refresh_min,
              out->category,
              out->lang,
              out->country,
              (unsigned)out->max_items,
              (unsigned)out->max_age_days);
    return ESP_OK;
}

esp_err_t news_settings_save(const news_settings_t *settings)
{
    if (!news_settings_validate(settings)) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        NEWS_LOGW("nvs open write failed: %s", esp_err_to_name(err));
        return err;
    }

    NEWS_LOGI("settings save provider=%s enabled=%d key_set=%d key_len=%u refresh=%d category=%s lang=%s country=%s max=%u age_days=%u",
              settings->provider,
              settings->enabled,
              settings->gnews_key[0] != '\0',
              (unsigned)strnlen(settings->gnews_key, sizeof(settings->gnews_key)),
              (int)settings->refresh_min,
              settings->category,
              settings->lang,
              settings->country,
              (unsigned)settings->max_items,
              (unsigned)settings->max_age_days);

    err = nvs_set_str(handle, NVS_KEY_PROVIDER, settings->provider);
    if (err == ESP_OK && settings->gnews_key[0] != '\0') {
        err = nvs_set_str(handle, NVS_KEY_GNEWS_KEY, settings->gnews_key);
    } else if (err == ESP_OK) {
        err = nvs_erase_key(handle, NVS_KEY_GNEWS_KEY);
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            err = ESP_OK;
        }
    }
    if (err == ESP_OK) {
        err = nvs_set_u8(handle, NVS_KEY_ENABLED, settings->enabled ? 1 : 0);
    }
    if (err == ESP_OK) {
        err = nvs_set_i16(handle, NVS_KEY_REFRESH_MIN, settings->refresh_min);
    }
    if (err == ESP_OK) {
        err = nvs_set_str(handle, NVS_KEY_CATEGORY, settings->category);
    }
    if (err == ESP_OK) {
        err = nvs_set_str(handle, NVS_KEY_LANG, settings->lang);
    }
    if (err == ESP_OK) {
        err = nvs_set_str(handle, NVS_KEY_COUNTRY, settings->country);
    }
    if (err == ESP_OK) {
        err = nvs_set_u8(handle, NVS_KEY_MAX_ITEMS, settings->max_items);
    }
    if (err == ESP_OK) {
        err = nvs_set_u8(handle, NVS_KEY_MAX_AGE_DAYS, settings->max_age_days);
    }
    if (err == ESP_OK) {
        err = nvs_set_i64(handle, NVS_KEY_LAST_OK_TS, settings->last_ok_ts);
    }
    if (err == ESP_OK) {
        err = nvs_set_str(handle, NVS_KEY_LAST_ERROR, settings->last_error);
    }
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    if (err != ESP_OK) {
        NEWS_LOGW("settings save failed: %s", esp_err_to_name(err));
    }
    nvs_close(handle);
    return err;
}

esp_err_t news_settings_clear(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        NEWS_LOGI("clear skipped: namespace missing");
        return ESP_OK;
    }
    if (err != ESP_OK) {
        NEWS_LOGW("clear open failed: %s", esp_err_to_name(err));
        return err;
    }

    NEWS_LOGI("clear all news settings");
    err = nvs_erase_all(handle);
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    if (err != ESP_OK) {
        NEWS_LOGW("clear failed: %s", esp_err_to_name(err));
    }
    nvs_close(handle);
    return err;
}
