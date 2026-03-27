#include "pulsemon_api_client.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cJSON.h"
#include "esp_http_client.h"
#include "esp_log.h"

#include "pulsemon_api_config.h"

static const char *TAG = "pulsemon_api";
static const size_t DASHBOARD_BODY_CAP = 8192;

typedef struct {
    char *buf;
    size_t cap;
    size_t len;
} http_acc_t;

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    http_acc_t *acc = (http_acc_t *)evt->user_data;
    if (acc == NULL) {
        return ESP_OK;
    }

    if (evt->event_id == HTTP_EVENT_ON_DATA && evt->data && evt->data_len > 0) {
        size_t copy = (size_t)evt->data_len;
        if (acc->len + copy >= acc->cap) {
            if (acc->len + 1 >= acc->cap) {
                return ESP_OK;
            }
            copy = acc->cap - acc->len - 1;
        }
        memcpy(acc->buf + acc->len, evt->data, copy);
        acc->len += copy;
        acc->buf[acc->len] = '\0';
    }
    return ESP_OK;
}

static void set_err(char *err, size_t err_len, const char *msg)
{
    if (err == NULL || err_len == 0) {
        return;
    }
    snprintf(err, err_len, "%s", msg ? msg : "unknown");
    err[err_len - 1] = '\0';
}

static cJSON *obj_get(cJSON *obj, const char *name)
{
    if (obj == NULL || name == NULL) {
        return NULL;
    }
    return cJSON_GetObjectItemCaseSensitive(obj, name);
}

static void parse_metric(cJSON *metric, float *value, bool *valid)
{
    if (value == NULL || valid == NULL) {
        return;
    }
    *valid = false;

    if (cJSON_IsNumber(metric)) {
        *value = (float)metric->valuedouble;
        *valid = true;
        return;
    }

    if (!cJSON_IsObject(metric)) {
        return;
    }

    cJSON *display = obj_get(metric, "value_display");
    if (cJSON_IsNumber(display)) {
        *value = (float)display->valuedouble;
        *valid = true;
        return;
    }

    cJSON *raw = obj_get(metric, "value_raw");
    if (cJSON_IsNumber(raw)) {
        *value = (float)raw->valuedouble;
        *valid = true;
    }
}

static void parse_metric_u64(cJSON *metric, unsigned long long *value, bool *valid)
{
    if (value == NULL || valid == NULL) {
        return;
    }
    *valid = false;

    if (cJSON_IsNumber(metric)) {
        *value = (unsigned long long)metric->valuedouble;
        *valid = true;
        return;
    }

    if (!cJSON_IsObject(metric)) {
        return;
    }

    cJSON *display = obj_get(metric, "value_display");
    if (cJSON_IsNumber(display)) {
        *value = (unsigned long long)display->valuedouble;
        *valid = true;
        return;
    }

    cJSON *raw = obj_get(metric, "value_raw");
    if (cJSON_IsNumber(raw)) {
        *value = (unsigned long long)raw->valuedouble;
        *valid = true;
    }
}

bool pulsemon_fetch_dashboard(pulsemon_dashboard_t *out, char *err, size_t err_len)
{
    if (out == NULL) {
        set_err(err, err_len, "out=null");
        return false;
    }
    memset(out, 0, sizeof(*out));

    char *body = (char *)calloc(1, DASHBOARD_BODY_CAP);
    if (body == NULL) {
        set_err(err, err_len, "oom_body");
        return false;
    }

    http_acc_t acc = {
        .buf = body,
        .cap = DASHBOARD_BODY_CAP,
        .len = 0,
    };

    char url[160];
    snprintf(url, sizeof(url), "%s/dashboard", PULSEMON_API_BASE_URL);

    esp_http_client_config_t cfg = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = PULSEMON_HTTP_TIMEOUT_MS,
        .event_handler = http_event_handler,
        .user_data = &acc,
        .buffer_size = 2048,
        .buffer_size_tx = 1024,
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (client == NULL) {
        set_err(err, err_len, "http_init_failed");
        free(body);
        return false;
    }

    esp_err_t rc = esp_http_client_perform(client);
    if (rc != ESP_OK) {
        ESP_LOGW(TAG, "dashboard request failed: %s", esp_err_to_name(rc));
        set_err(err, err_len, "http_perform_failed");
        esp_http_client_cleanup(client);
        free(body);
        return false;
    }

    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    if (status != 200) {
        set_err(err, err_len, "http_status_not_200");
        free(body);
        return false;
    }

    cJSON *root = cJSON_Parse(body);
    if (!cJSON_IsObject(root)) {
        set_err(err, err_len, "json_parse_failed");
        if (root) {
            cJSON_Delete(root);
        }
        free(body);
        return false;
    }

    cJSON *host = obj_get(root, "host");
    if (cJSON_IsString(host) && host->valuestring != NULL) {
        snprintf(out->host, sizeof(out->host), "%s", host->valuestring);
        out->host[sizeof(out->host) - 1] = '\0';
        out->host_valid = true;
    }

    cJSON *cpu = obj_get(root, "cpu");
    cJSON *mem = obj_get(root, "mem");
    cJSON *gpu = obj_get(root, "gpu");
    cJSON *state = obj_get(root, "state");

    if (cJSON_IsObject(cpu)) {
        parse_metric(obj_get(cpu, "pct"), &out->cpu_pct, &out->cpu_pct_valid);
        parse_metric(obj_get(cpu, "temp_c"), &out->cpu_temp_c, &out->cpu_temp_c_valid);
    }
    if (cJSON_IsObject(mem)) {
        parse_metric(obj_get(mem, "pct"), &out->mem_pct, &out->mem_pct_valid);
        parse_metric_u64(obj_get(mem, "used_b"), &out->mem_used_b, &out->mem_used_b_valid);
        parse_metric_u64(obj_get(mem, "total_b"), &out->mem_total_b, &out->mem_total_b_valid);
    }
    if (cJSON_IsObject(gpu)) {
        parse_metric(obj_get(gpu, "pct"), &out->gpu_pct, &out->gpu_pct_valid);
        parse_metric(obj_get(gpu, "temp_c"), &out->gpu_temp_c, &out->gpu_temp_c_valid);
        parse_metric(obj_get(gpu, "power_w"), &out->gpu_power_w, &out->gpu_power_w_valid);
    }
    if (cJSON_IsObject(state)) {
        cJSON *ok = obj_get(state, "ok");
        if (cJSON_IsBool(ok)) {
            out->state_ok = cJSON_IsTrue(ok);
            out->state_ok_valid = true;
        }
        cJSON *stale = obj_get(state, "stale_ms");
        if (cJSON_IsNumber(stale)) {
            out->stale_ms = stale->valueint;
            out->stale_ms_valid = true;
        }
    }

    cJSON_Delete(root);
    free(body);
    return true;
}
