#include "wifi_credentials.h"

#include <string.h>

#include "nvs.h"
#include "nvs_flash.h"

static const char *NVS_NAMESPACE = "pulsemon_wifi";
static const char *NVS_KEY_SSID = "ssid";
static const char *NVS_KEY_PASSWORD = "password";

static bool valid_ssid(const char *ssid)
{
    if (ssid == NULL) {
        return false;
    }

    size_t len = strnlen(ssid, PULSEMON_WIFI_SSID_MAX_LEN + 1);
    return len > 0 && len <= PULSEMON_WIFI_SSID_MAX_LEN;
}

static bool valid_password(const char *password)
{
    if (password == NULL) {
        return true;
    }

    size_t len = strnlen(password, PULSEMON_WIFI_PASSWORD_MAX_LEN + 1);
    return len == 0 || (len >= 8 && len < PULSEMON_WIFI_PASSWORD_MAX_LEN);
}

bool pulsemon_wifi_credentials_validate(const char *ssid, const char *password)
{
    return valid_ssid(ssid) && valid_password(password);
}

esp_err_t pulsemon_wifi_credentials_load(pulsemon_wifi_credentials_t *out)
{
    if (out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    memset(out, 0, sizeof(*out));

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        return err;
    }

    size_t ssid_len = sizeof(out->ssid);
    err = nvs_get_str(handle, NVS_KEY_SSID, out->ssid, &ssid_len);
    if (err != ESP_OK) {
        nvs_close(handle);
        return err;
    }

    size_t password_len = sizeof(out->password);
    err = nvs_get_str(handle, NVS_KEY_PASSWORD, out->password, &password_len);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        out->password[0] = '\0';
        err = ESP_OK;
    }

    nvs_close(handle);
    if (err != ESP_OK) {
        return err;
    }
    return pulsemon_wifi_credentials_validate(out->ssid, out->password) ? ESP_OK : ESP_ERR_INVALID_SIZE;
}

esp_err_t pulsemon_wifi_credentials_save(const char *ssid, const char *password)
{
    if (!pulsemon_wifi_credentials_validate(ssid, password)) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_set_str(handle, NVS_KEY_SSID, ssid);
    if (err == ESP_OK) {
        err = nvs_set_str(handle, NVS_KEY_PASSWORD, password ? password : "");
    }
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err;
}

esp_err_t pulsemon_wifi_credentials_clear(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return ESP_OK;
    }
    if (err != ESP_OK) {
        return err;
    }

    esp_err_t erase_ssid = nvs_erase_key(handle, NVS_KEY_SSID);
    esp_err_t erase_password = nvs_erase_key(handle, NVS_KEY_PASSWORD);
    err = nvs_commit(handle);
    nvs_close(handle);

    if (err != ESP_OK) {
        return err;
    }
    if (erase_ssid != ESP_OK && erase_ssid != ESP_ERR_NVS_NOT_FOUND) {
        return erase_ssid;
    }
    if (erase_password != ESP_OK && erase_password != ESP_ERR_NVS_NOT_FOUND) {
        return erase_password;
    }
    return ESP_OK;
}

bool pulsemon_wifi_credentials_present(void)
{
    pulsemon_wifi_credentials_t credentials;
    return pulsemon_wifi_credentials_load(&credentials) == ESP_OK;
}
