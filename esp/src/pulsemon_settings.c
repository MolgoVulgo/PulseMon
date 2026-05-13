#include "pulsemon_settings.h"

#include <stdio.h>
#include <string.h>

#include "nvs.h"

static const char *NVS_NAMESPACE = "pulsemon_cfg";
static const char *NVS_KEY_OPENWEATHER = "ow_key";
static const char *NVS_KEY_GMT_OFFSET = "gmt_min";
static const char *NVS_KEY_CITY_ID = "ow_city";
static const char *NVS_KEY_LANGUAGE = "lang";

static bool language_is_valid(const char *language)
{
    static const char *valid[] = {"fr", "en", "de", "es", "it"};

    if (language == NULL) {
        return false;
    }
    for (size_t i = 0; i < sizeof(valid) / sizeof(valid[0]); i++) {
        if (strcmp(language, valid[i]) == 0) {
            return true;
        }
    }
    return false;
}

void pulsemon_settings_defaults(pulsemon_settings_t *out)
{
    if (out == NULL) {
        return;
    }
    memset(out, 0, sizeof(*out));
    out->gmt_offset_min = 60;
    snprintf(out->language, sizeof(out->language), "fr");
}

bool pulsemon_settings_validate(const pulsemon_settings_t *settings)
{
    if (settings == NULL) {
        return false;
    }

    if (strnlen(settings->openweather_key, sizeof(settings->openweather_key)) >= sizeof(settings->openweather_key)) {
        return false;
    }
    if (settings->gmt_offset_min < -720 || settings->gmt_offset_min > 840) {
        return false;
    }
    if (!language_is_valid(settings->language)) {
        return false;
    }
    return true;
}

esp_err_t pulsemon_settings_load(pulsemon_settings_t *out)
{
    if (out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    pulsemon_settings_defaults(out);

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return ESP_OK;
    }
    if (err != ESP_OK) {
        return err;
    }

    size_t key_len = sizeof(out->openweather_key);
    err = nvs_get_str(handle, NVS_KEY_OPENWEATHER, out->openweather_key, &key_len);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        nvs_close(handle);
        return err;
    }

    int16_t gmt_offset = out->gmt_offset_min;
    err = nvs_get_i16(handle, NVS_KEY_GMT_OFFSET, &gmt_offset);
    if (err == ESP_OK) {
        out->gmt_offset_min = gmt_offset;
    } else if (err != ESP_ERR_NVS_NOT_FOUND) {
        nvs_close(handle);
        return err;
    }

    uint32_t city_id = out->openweather_city_id;
    err = nvs_get_u32(handle, NVS_KEY_CITY_ID, &city_id);
    if (err == ESP_OK) {
        out->openweather_city_id = city_id;
    } else if (err != ESP_ERR_NVS_NOT_FOUND) {
        nvs_close(handle);
        return err;
    }

    size_t language_len = sizeof(out->language);
    err = nvs_get_str(handle, NVS_KEY_LANGUAGE, out->language, &language_len);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        nvs_close(handle);
        return err;
    }

    nvs_close(handle);
    if (!pulsemon_settings_validate(out)) {
        pulsemon_settings_defaults(out);
        return ESP_ERR_INVALID_SIZE;
    }
    return ESP_OK;
}

esp_err_t pulsemon_settings_save(const pulsemon_settings_t *settings)
{
    if (!pulsemon_settings_validate(settings)) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }

    if (settings->openweather_key[0] != '\0') {
        err = nvs_set_str(handle, NVS_KEY_OPENWEATHER, settings->openweather_key);
    } else {
        err = nvs_erase_key(handle, NVS_KEY_OPENWEATHER);
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            err = ESP_OK;
        }
    }
    if (err == ESP_OK) {
        err = nvs_set_i16(handle, NVS_KEY_GMT_OFFSET, settings->gmt_offset_min);
    }
    if (err == ESP_OK) {
        err = nvs_set_u32(handle, NVS_KEY_CITY_ID, settings->openweather_city_id);
    }
    if (err == ESP_OK) {
        err = nvs_set_str(handle, NVS_KEY_LANGUAGE, settings->language);
    }
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err;
}

esp_err_t pulsemon_settings_clear(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return ESP_OK;
    }
    if (err != ESP_OK) {
        return err;
    }

    esp_err_t erase_all = nvs_erase_all(handle);
    if (erase_all == ESP_OK) {
        erase_all = nvs_commit(handle);
    }
    nvs_close(handle);
    return erase_all;
}
