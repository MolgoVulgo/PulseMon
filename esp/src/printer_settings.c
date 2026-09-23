#include "printer_settings.h"

#include <ctype.h>
#include <string.h>

#include "nvs.h"

static const char *NVS_NAMESPACE = "printer";
static const char *NVS_KEY_HOST = "host";
static const char *NVS_KEY_ACCESS_CODE = "access_code";

static bool host_is_valid(const char *host)
{
    if (host == NULL) {
        return false;
    }

    size_t len = strnlen(host, PRINTER_HOST_MAX_LEN + 1);
    if (len == 0 || len > PRINTER_HOST_MAX_LEN) {
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

static bool access_code_is_valid(const char *access_code)
{
    if (access_code == NULL) {
        return false;
    }

    size_t len = strnlen(access_code, PRINTER_ACCESS_CODE_MAX_LEN + 1);
    if (len == 0 || len > PRINTER_ACCESS_CODE_MAX_LEN) {
        return false;
    }

    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)access_code[i];
        if (c <= 0x20 || c == 0x7f) {
            return false;
        }
    }
    return true;
}

void printer_settings_defaults(printer_settings_t *out)
{
    if (out == NULL) {
        return;
    }
    memset(out, 0, sizeof(*out));
}

bool printer_settings_validate(const printer_settings_t *settings)
{
    if (settings == NULL) {
        return false;
    }

    bool host_set = settings->host[0] != '\0';
    bool access_code_set = settings->access_code[0] != '\0';
    if (!host_set && !access_code_set) {
        return true;
    }
    if (host_set != access_code_set) {
        return false;
    }
    return host_is_valid(settings->host) && access_code_is_valid(settings->access_code);
}

esp_err_t printer_settings_load(printer_settings_t *out)
{
    if (out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    printer_settings_defaults(out);

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

    size_t access_code_len = sizeof(out->access_code);
    err = nvs_get_str(handle, NVS_KEY_ACCESS_CODE, out->access_code, &access_code_len);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        nvs_close(handle);
        return err;
    }

    nvs_close(handle);
    if (!printer_settings_validate(out)) {
        printer_settings_defaults(out);
        return ESP_ERR_INVALID_SIZE;
    }
    return ESP_OK;
}

esp_err_t printer_settings_save(const printer_settings_t *settings)
{
    if (!printer_settings_validate(settings)) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_set_str(handle, NVS_KEY_HOST, settings->host);
    if (err == ESP_OK) {
        err = nvs_set_str(handle, NVS_KEY_ACCESS_CODE, settings->access_code);
    }
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err;
}

esp_err_t printer_settings_clear(void)
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
