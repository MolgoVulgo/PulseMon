#include "pulsemon_api_settings.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "nvs.h"

#include "pulsemon_api_config.h"

static const char *NVS_NAMESPACE = "pulsemon_api";
static const char *NVS_KEY_HOST = "host";
static const char *NVS_KEY_PORT = "port";

static bool host_is_valid(const char *host)
{
    if (host == NULL) {
        return false;
    }

    size_t len = strnlen(host, PULSEMON_API_HOST_MAX_LEN + 1);
    if (len == 0 || len > PULSEMON_API_HOST_MAX_LEN) {
        return false;
    }

    bool label_start = true;
    char previous = '\0';
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)host[i];
        if (isalnum(c)) {
            label_start = false;
        } else if (c == '-') {
            if (label_start) {
                return false;
            }
        } else if (c == '.') {
            if (label_start || previous == '-') {
                return false;
            }
            label_start = true;
        } else {
            return false;
        }
        previous = (char)c;
    }

    return !label_start && previous != '-';
}

void pulsemon_api_settings_defaults(pulsemon_api_settings_t *out)
{
    if (out == NULL) {
        return;
    }
    memset(out, 0, sizeof(*out));
    snprintf(out->host, sizeof(out->host), "%s", PULSEMON_API_DEFAULT_HOST);
    out->port = PULSEMON_API_DEFAULT_PORT;
}

bool pulsemon_api_settings_validate(const pulsemon_api_settings_t *settings)
{
    if (settings == NULL) {
        return false;
    }
    return host_is_valid(settings->host) && settings->port > 0;
}

esp_err_t pulsemon_api_settings_load(pulsemon_api_settings_t *out)
{
    if (out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    pulsemon_api_settings_defaults(out);

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return ESP_OK;
    }
    if (err != ESP_OK) {
        return err;
    }

    size_t host_len = sizeof(out->host);
    err = nvs_get_str(handle, NVS_KEY_HOST, out->host, &host_len);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        nvs_close(handle);
        return err;
    }

    uint16_t port = out->port;
    err = nvs_get_u16(handle, NVS_KEY_PORT, &port);
    if (err == ESP_OK) {
        out->port = port;
    } else if (err != ESP_ERR_NVS_NOT_FOUND) {
        nvs_close(handle);
        return err;
    }

    nvs_close(handle);
    if (!pulsemon_api_settings_validate(out)) {
        pulsemon_api_settings_defaults(out);
        return ESP_ERR_INVALID_SIZE;
    }
    return ESP_OK;
}

esp_err_t pulsemon_api_settings_save(const pulsemon_api_settings_t *settings)
{
    if (!pulsemon_api_settings_validate(settings)) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_set_str(handle, NVS_KEY_HOST, settings->host);
    if (err == ESP_OK) {
        err = nvs_set_u16(handle, NVS_KEY_PORT, settings->port);
    }
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err;
}

esp_err_t pulsemon_api_settings_clear(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return ESP_OK;
    }
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_erase_all(handle);
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err;
}

bool pulsemon_api_settings_build_base_url(
    const pulsemon_api_settings_t *settings,
    char *out,
    size_t out_len)
{
    if (out == NULL || out_len == 0 || !pulsemon_api_settings_validate(settings)) {
        return false;
    }

    int written = snprintf(out, out_len, "http://%s:%u/api/v1", settings->host, (unsigned)settings->port);
    return written > 0 && (size_t)written < out_len;
}
