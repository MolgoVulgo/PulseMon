#include "news_service.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cJSON.h"
#include "esp_bsp.h"
#include "esp_crt_bundle.h"
#include "esp_err.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "news_settings.h"
#include "vars.h"

#ifndef PULSEMON_NEWS_DEBUG
#define PULSEMON_NEWS_DEBUG 0
#endif

#if PULSEMON_NEWS_DEBUG
static const char *TAG = "pulsemon_news";
#define NEWS_LOGI(fmt, ...) ESP_LOGI(TAG, fmt, ##__VA_ARGS__)
#define NEWS_LOGW(fmt, ...) ESP_LOGW(TAG, fmt, ##__VA_ARGS__)
#else
#define NEWS_LOGI(fmt, ...) ((void)0)
#define NEWS_LOGW(fmt, ...) ((void)0)
#endif

#define NEWS_HTTP_TIMEOUT_MS 8000
#define NEWS_BODY_CAP 16384
#define NEWS_TITLE_MAX_LEN 160
#define NEWS_SOURCE_MAX_LEN 48
#define NEWS_VALID_EPOCH_MIN 1609459200L
#define NEWS_CACHE_STALE_SECONDS (6 * 60 * 60)
#define NEWS_RETRY_BACKOFF_SECONDS (30 * 60)
#define NEWS_LONG_BACKOFF_SECONDS (6 * 60 * 60)

typedef struct {
    char *buf;
    size_t cap;
    size_t len;
    bool overflow;
} news_http_acc_t;

typedef struct {
    bool valid;
    char title[NEWS_TITLE_MAX_LEN + 1];
    char source[NEWS_SOURCE_MAX_LEN + 1];
    time_t published_ts;
    time_t fetch_ts;
} news_cache_t;

static TaskHandle_t s_news_task;
static esp_timer_handle_t s_news_timer;
static bool s_started;
static news_cache_t s_cache;
static time_t s_next_allowed_fetch;

static bool netif_ready(void)
{
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif == NULL) {
        return false;
    }
    esp_netif_ip_info_t ip_info;
    if (esp_netif_get_ip_info(netif, &ip_info) != ESP_OK) {
        return false;
    }
    return ip_info.ip.addr != 0;
}

static bool time_ready(time_t *now_out)
{
    time_t now = 0;
    time(&now);
    if (now < NEWS_VALID_EPOCH_MIN) {
        return false;
    }
    if (now_out != NULL) {
        *now_out = now;
    }
    return true;
}

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    news_http_acc_t *acc = (news_http_acc_t *)evt->user_data;
    if (acc == NULL) {
        return ESP_OK;
    }
    if (evt->event_id == HTTP_EVENT_ON_DATA && evt->data != NULL && evt->data_len > 0) {
        size_t copy = (size_t)evt->data_len;
        if (acc->len + copy >= acc->cap) {
            acc->overflow = true;
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

static cJSON *obj_get(cJSON *obj, const char *name)
{
    if (obj == NULL || name == NULL) {
        return NULL;
    }
    return cJSON_GetObjectItemCaseSensitive(obj, name);
}

static const char *json_string_or(cJSON *obj, const char *name)
{
    cJSON *item = obj_get(obj, name);
    return cJSON_IsString(item) ? item->valuestring : NULL;
}

static bool parse_iso_utc(const char *value, time_t *out)
{
    int year = 0;
    int mon = 0;
    int mday = 0;
    int hour = 0;
    int min = 0;
    int sec = 0;
    if (value == NULL || out == NULL) {
        return false;
    }
    if (sscanf(value, "%d-%d-%dT%d:%d:%dZ", &year, &mon, &mday, &hour, &min, &sec) != 6) {
        return false;
    }
    struct tm tm_utc = {
        .tm_year = year - 1900,
        .tm_mon = mon - 1,
        .tm_mday = mday,
        .tm_hour = hour,
        .tm_min = min,
        .tm_sec = sec,
    };
    char *old_tz = getenv("TZ");
    setenv("TZ", "UTC0", 1);
    tzset();
    *out = mktime(&tm_utc);
    if (old_tz != NULL) {
        setenv("TZ", old_tz, 1);
    } else {
        unsetenv("TZ");
    }
    tzset();
    return *out >= NEWS_VALID_EPOCH_MIN;
}

static bool utf8_valid(const char *s)
{
    const unsigned char *p = (const unsigned char *)s;
    while (p != NULL && *p != '\0') {
        if (*p < 0x80) {
            p++;
        } else if ((*p & 0xE0) == 0xC0 && (p[1] & 0xC0) == 0x80) {
            p += 2;
        } else if ((*p & 0xF0) == 0xE0 && (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80) {
            p += 3;
        } else if ((*p & 0xF8) == 0xF0 && (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80 && (p[3] & 0xC0) == 0x80) {
            p += 4;
        } else {
            return false;
        }
    }
    return true;
}

static void clean_title(const char *in, char *out, size_t out_len)
{
    bool last_space = true;
    size_t di = 0;
    if (out_len == 0) {
        return;
    }
    for (size_t si = 0; in != NULL && in[si] != '\0' && di + 1 < out_len; si++) {
        unsigned char c = (unsigned char)in[si];
        if (c == '\r' || c == '\n' || c == '\t' || c == ' ') {
            if (!last_space) {
                out[di++] = ' ';
                last_space = true;
            }
            continue;
        }
        out[di++] = (char)c;
        last_space = false;
    }
    if (di > 0 && out[di - 1] == ' ') {
        di--;
    }
    out[di] = '\0';
}

static bool title_noisy(const char *title)
{
    return title == NULL || strstr(title, "http://") != NULL || strstr(title, "https://") != NULL;
}

static bool parse_gnews_json(const char *json, const news_settings_t *settings, time_t now, news_cache_t *out)
{
    cJSON *root = cJSON_Parse(json);
    if (!cJSON_IsObject(root)) {
        if (root != NULL) {
            cJSON_Delete(root);
        }
        NEWS_LOGW("json parse failed");
        return false;
    }

    cJSON *total = obj_get(root, "totalArticles");
    NEWS_LOGI("json totalArticles=%d", cJSON_IsNumber(total) ? total->valueint : -1);
#if !PULSEMON_NEWS_DEBUG
    (void)total;
#endif

    cJSON *articles = obj_get(root, "articles");
    if (!cJSON_IsArray(articles)) {
        cJSON_Delete(root);
        NEWS_LOGW("json missing articles array");
        return false;
    }

    time_t cutoff = now - ((time_t)settings->max_age_days * 86400);
    int count = cJSON_GetArraySize(articles);
    for (int i = 0; i < count; i++) {
        cJSON *article = cJSON_GetArrayItem(articles, i);
        if (!cJSON_IsObject(article)) {
            continue;
        }
        const char *title = json_string_or(article, "title");
        const char *published = json_string_or(article, "publishedAt");
        const char *lang = json_string_or(article, "lang");
        if (title == NULL || strlen(title) <= 10 || published == NULL || title_noisy(title) || !utf8_valid(title)) {
            NEWS_LOGI("article[%d] ignored invalid title", i);
            continue;
        }
        if (lang != NULL && strcmp(lang, settings->lang) != 0) {
            NEWS_LOGI("article[%d] ignored lang=%s", i, lang);
            continue;
        }
        time_t published_ts = 0;
        if (!parse_iso_utc(published, &published_ts) || published_ts < cutoff) {
            NEWS_LOGI("article[%d] ignored publishedAt=%s", i, published);
            continue;
        }

        memset(out, 0, sizeof(*out));
        clean_title(title, out->title, sizeof(out->title));
        cJSON *source = obj_get(article, "source");
        if (cJSON_IsObject(source)) {
            const char *source_name = json_string_or(source, "name");
            if (source_name != NULL) {
                clean_title(source_name, out->source, sizeof(out->source));
            }
        }
        out->published_ts = published_ts;
        out->fetch_ts = now;
        out->valid = out->title[0] != '\0';
        NEWS_LOGI("article[%d] selected title_len=%u source=%s", i, (unsigned)strlen(out->title), out->source);
        cJSON_Delete(root);
        return out->valid;
    }

    cJSON_Delete(root);
    NEWS_LOGW("empty_result: no valid article in %d item(s)", count);
    return false;
}

static bool build_gnews_url(const news_settings_t *settings, time_t now, char *url, size_t url_len)
{
    time_t from = now - ((time_t)settings->max_age_days * 86400);
    struct tm tm_utc;
    if (gmtime_r(&from, &tm_utc) == NULL) {
        return false;
    }
    char from_iso[32];
    if (strftime(from_iso, sizeof(from_iso), "%Y-%m-%dT%H:%M:%SZ", &tm_utc) == 0) {
        return false;
    }
    snprintf(url,
             url_len,
             "https://gnews.io/api/v4/top-headlines?category=%s&lang=%s&country=%s&max=%u&from=%s",
             settings->category,
             settings->lang,
             settings->country,
             (unsigned)settings->max_items,
             from_iso);
    return true;
}

static bool fetch_gnews(const news_settings_t *settings, time_t now, char *body, size_t body_len, int *http_status)
{
    char url[256];
    if (!build_gnews_url(settings, now, url, sizeof(url))) {
        NEWS_LOGW("url build failed");
        return false;
    }
    body[0] = '\0';
    news_http_acc_t acc = {
        .buf = body,
        .cap = body_len,
        .len = 0,
        .overflow = false,
    };
    esp_http_client_config_t cfg = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = NEWS_HTTP_TIMEOUT_MS,
        .event_handler = http_event_handler,
        .user_data = &acc,
        .buffer_size = 1024,
        .buffer_size_tx = 512,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    NEWS_LOGI("request GET %s", url);
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (client == NULL) {
        NEWS_LOGW("http init failed");
        return false;
    }
    esp_http_client_set_header(client, "X-Api-Key", settings->gnews_key);
    esp_http_client_set_header(client, "User-Agent", "PulseMonESP32");
    esp_http_client_set_header(client, "Accept", "application/json");

    int64_t started_us = esp_timer_get_time();
    esp_err_t err = esp_http_client_perform(client);
    int elapsed_ms = (int)((esp_timer_get_time() - started_us) / 1000);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    if (http_status != NULL) {
        *http_status = status;
    }
    NEWS_LOGI("response err=%s status=%d bytes=%u elapsed_ms=%d", esp_err_to_name(err), status, (unsigned)acc.len, elapsed_ms);
#if !PULSEMON_NEWS_DEBUG
    (void)elapsed_ms;
#endif
    if (err != ESP_OK) {
        NEWS_LOGW("https request failed: %s", esp_err_to_name(err));
        return false;
    }
    if (status != 200) {
        NEWS_LOGW("http status=%d", status);
        return false;
    }
    if (acc.overflow) {
        NEWS_LOGW("payload truncated cap=%u", (unsigned)body_len);
        return false;
    }
    return acc.len > 0;
}

static void apply_info_line(const char *text)
{
    if (bsp_display_lock(pdMS_TO_TICKS(100))) {
        set_var_ui_meteo_alert(text != NULL ? text : "");
        bsp_display_unlock();
    }
}

static void apply_cache_if_available(time_t now)
{
    if (s_cache.valid && s_cache.fetch_ts > 0 && now - s_cache.fetch_ts <= NEWS_CACHE_STALE_SECONDS) {
        apply_info_line(s_cache.title);
    } else {
        apply_info_line("");
    }
}

static void fetch_news_once(void)
{
    news_settings_t settings;
    esp_err_t err = news_settings_load(&settings);
    if (err != ESP_OK && err != ESP_ERR_INVALID_SIZE) {
        NEWS_LOGW("settings load failed: %s", esp_err_to_name(err));
        return;
    }

    time_t now = 0;
    if (!time_ready(&now)) {
        NEWS_LOGW("skip time_invalid");
        apply_cache_if_available(now);
        return;
    }
    if (!settings.enabled) {
        NEWS_LOGI("skip disabled");
        apply_info_line("");
        return;
    }
    if (settings.gnews_key[0] == '\0') {
        NEWS_LOGI("skip missing_key");
        apply_cache_if_available(now);
        return;
    }
    if (!netif_ready()) {
        NEWS_LOGW("skip wifi_down");
        apply_cache_if_available(now);
        return;
    }
    if (now < s_next_allowed_fetch) {
        NEWS_LOGI("skip backoff remaining=%llds", (long long)(s_next_allowed_fetch - now));
        apply_cache_if_available(now);
        return;
    }

    char *body = (char *)calloc(1, NEWS_BODY_CAP);
    if (body == NULL) {
        NEWS_LOGW("body alloc failed");
        apply_cache_if_available(now);
        return;
    }

    int status = 0;
    news_cache_t fresh = {0};
    if (fetch_gnews(&settings, now, body, NEWS_BODY_CAP, &status) &&
        parse_gnews_json(body, &settings, now, &fresh)) {
        s_cache = fresh;
        s_next_allowed_fetch = now + ((time_t)settings.refresh_min * 60);
        apply_info_line(s_cache.title);
        NEWS_LOGI("cache updated next_fetch_in=%dm", (int)settings.refresh_min);
        free(body);
        return;
    }

    if (status == 401 || status == 400) {
        s_next_allowed_fetch = now + NEWS_LONG_BACKOFF_SECONDS;
    } else if (status == 403 || status == 429) {
        s_next_allowed_fetch = now + NEWS_LONG_BACKOFF_SECONDS;
    } else {
        s_next_allowed_fetch = now + NEWS_RETRY_BACKOFF_SECONDS;
    }
    apply_cache_if_available(now);
    NEWS_LOGW("update failed status=%d next_retry_in=%llds", status, (long long)(s_next_allowed_fetch - now));
    free(body);
}

static void news_task(void *arg)
{
    (void)arg;
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        fetch_news_once();
    }
}

static void news_timer_cb(void *arg)
{
    (void)arg;
    news_service_request_update();
}

esp_err_t news_service_start(void)
{
    if (s_started) {
        return ESP_OK;
    }
    if (s_news_task == NULL) {
        BaseType_t ok = xTaskCreate(news_task, "NewsTask", 8192, NULL, tskIDLE_PRIORITY + 1, &s_news_task);
        if (ok != pdPASS) {
            s_news_task = NULL;
            return ESP_ERR_NO_MEM;
        }
    }
    esp_timer_create_args_t timer_args = {
        .callback = news_timer_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "news_refresh",
        .skip_unhandled_events = true,
    };
    esp_err_t err = esp_timer_create(&timer_args, &s_news_timer);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_timer_start_periodic(s_news_timer, 60LL * 1000000LL);
    if (err != ESP_OK) {
        return err;
    }
    s_started = true;
    news_service_request_update();
    NEWS_LOGI("service started");
    return ESP_OK;
}

void news_service_request_update(void)
{
    if (s_news_task != NULL) {
        xTaskNotifyGive(s_news_task);
    }
}
