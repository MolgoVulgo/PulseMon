#include "pulsemon_meteo_service.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cJSON.h"
#include "esp_bsp.h"
#include "esp_err.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "pulsemon_settings.h"
#include "pulsemon_weather_icons.h"
#include "ui_backend.h"
#include "vars.h"

#ifndef PULSEMON_DEBUG
#define PULSEMON_DEBUG 0
#endif

#if PULSEMON_DEBUG
static const char *TAG = "pulsemon_meteo";
#define METEO_LOGI(fmt, ...) ESP_LOGI(TAG, fmt, ##__VA_ARGS__)
#define METEO_LOGW(fmt, ...) ESP_LOGW(TAG, fmt, ##__VA_ARGS__)
#else
#define METEO_LOGI(fmt, ...) ((void)0)
#define METEO_LOGW(fmt, ...) ((void)0)
#endif

#define PULSEMON_METEO_REFRESH_MINUTES 30
#define PULSEMON_METEO_HTTP_TIMEOUT_MS 8000
#define PULSEMON_METEO_BODY_CAP 32768
#define PULSEMON_METEO_FORECAST_DAYS 6
#define PULSEMON_METEO_VALID_EPOCH_MIN 1609459200L

typedef struct {
    bool valid;
    int year;
    int yday;
    time_t timestamp;
    float min_c;
    float max_c;
    int condition_id;
    uint8_t icon_variant;
    uint8_t midday_offset;
} meteo_day_t;

typedef struct {
    bool has_current;
    bool has_coords;
    float current_temp_c;
    double lat;
    double lon;
    char condition[64];
    int current_condition_id;
    uint8_t current_icon_variant;
    meteo_day_t days[PULSEMON_METEO_FORECAST_DAYS];
} meteo_snapshot_t;

typedef struct {
    char *buf;
    size_t cap;
    size_t len;
    bool overflow;
} http_acc_t;

static TaskHandle_t s_meteo_task;
static TaskHandle_t s_clock_task;
static esp_timer_handle_t s_meteo_timer;
static bool s_started;
static meteo_snapshot_t s_last_snapshot;
static bool s_has_last_snapshot;

static const char *weekday_short(const char *language, int weekday)
{
    static const char *fr[] = {"DIM", "LUN", "MAR", "MER", "JEU", "VEN", "SAM"};
    static const char *en[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
    static const char *de[] = {"SO", "MO", "DI", "MI", "DO", "FR", "SA"};
    static const char *es[] = {"DOM", "LUN", "MAR", "MIE", "JUE", "VIE", "SAB"};
    static const char *it[] = {"DOM", "LUN", "MAR", "MER", "GIO", "VEN", "SAB"};
    const char **table = fr;

    if (weekday < 0 || weekday > 6) {
        return "--";
    }
    if (strcmp(language, "en") == 0) {
        table = en;
    } else if (strcmp(language, "de") == 0) {
        table = de;
    } else if (strcmp(language, "es") == 0) {
        table = es;
    } else if (strcmp(language, "it") == 0) {
        table = it;
    }
    return table[weekday];
}

static const char *weekday_long(const char *language, int weekday)
{
    static const char *fr[] = {"Dimanche", "Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi"};
    static const char *en[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    static const char *de[] = {"Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag"};
    static const char *es[] = {"Domingo", "Lunes", "Martes", "Miercoles", "Jueves", "Viernes", "Sabado"};
    static const char *it[] = {"Domenica", "Lunedi", "Martedi", "Mercoledi", "Giovedi", "Venerdi", "Sabato"};
    const char **table = fr;

    if (weekday < 0 || weekday > 6) {
        return "--";
    }
    if (strcmp(language, "en") == 0) {
        table = en;
    } else if (strcmp(language, "de") == 0) {
        table = de;
    } else if (strcmp(language, "es") == 0) {
        table = es;
    } else if (strcmp(language, "it") == 0) {
        table = it;
    }
    return table[weekday];
}

static const char *month_long(const char *language, int month)
{
    static const char *fr[] = {"Janvier", "Fevrier", "Mars", "Avril", "Mai", "Juin", "Juillet", "Aout", "Septembre", "Octobre", "Novembre", "Decembre"};
    static const char *en[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
    static const char *de[] = {"Januar", "Februar", "Marz", "April", "Mai", "Juni", "Juli", "August", "September", "Oktober", "November", "Dezember"};
    static const char *es[] = {"Enero", "Febrero", "Marzo", "Abril", "Mayo", "Junio", "Julio", "Agosto", "Septiembre", "Octubre", "Noviembre", "Diciembre"};
    static const char *it[] = {"Gennaio", "Febbraio", "Marzo", "Aprile", "Maggio", "Giugno", "Luglio", "Agosto", "Settembre", "Ottobre", "Novembre", "Dicembre"};
    const char **table = fr;

    if (month < 0 || month > 11) {
        return "";
    }
    if (strcmp(language, "en") == 0) {
        table = en;
    } else if (strcmp(language, "de") == 0) {
        table = de;
    } else if (strcmp(language, "es") == 0) {
        table = es;
    } else if (strcmp(language, "it") == 0) {
        table = it;
    }
    return table[month];
}

static bool local_tm_from_epoch(time_t epoch, int16_t gmt_offset_min, struct tm *out)
{
    if (out == NULL || epoch < PULSEMON_METEO_VALID_EPOCH_MIN) {
        return false;
    }
    time_t adjusted = epoch + ((time_t)gmt_offset_min * 60);
    return gmtime_r(&adjusted, out) != NULL;
}

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    http_acc_t *acc = (http_acc_t *)evt->user_data;
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

static bool fetch_url(const char *url, char *body, size_t body_len)
{
    if (url == NULL || body == NULL || body_len == 0) {
        return false;
    }
    body[0] = '\0';

    http_acc_t acc = {
        .buf = body,
        .cap = body_len,
        .len = 0,
        .overflow = false,
    };
    esp_http_client_config_t cfg = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = PULSEMON_METEO_HTTP_TIMEOUT_MS,
        .event_handler = http_event_handler,
        .user_data = &acc,
        .buffer_size = 1024,
        .buffer_size_tx = 512,
    };
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (client == NULL) {
        METEO_LOGW("http init failed");
        return false;
    }

    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    if (err != ESP_OK) {
        METEO_LOGW("http request failed: %s", esp_err_to_name(err));
        return false;
    }
    if (status != 200) {
        METEO_LOGW("http status=%d", status);
        return false;
    }
    if (acc.overflow) {
        METEO_LOGW("weather payload truncated");
        return false;
    }
    return acc.len > 0;
}

static cJSON *obj_get(cJSON *obj, const char *name)
{
    if (obj == NULL || name == NULL) {
        return NULL;
    }
    return cJSON_GetObjectItemCaseSensitive(obj, name);
}

static float json_float_or(cJSON *obj, const char *name, float fallback)
{
    cJSON *item = obj_get(obj, name);
    return cJSON_IsNumber(item) ? (float)item->valuedouble : fallback;
}

static int json_int_or(cJSON *obj, const char *name, int fallback)
{
    cJSON *item = obj_get(obj, name);
    return cJSON_IsNumber(item) ? item->valueint : fallback;
}

static const char *json_string_or(cJSON *obj, const char *name)
{
    cJSON *item = obj_get(obj, name);
    return cJSON_IsString(item) ? item->valuestring : NULL;
}

static uint8_t icon_variant_from_code(const char *icon)
{
    if (icon != NULL) {
        size_t len = strlen(icon);
        if (len > 0 && icon[len - 1] == 'd') {
            return 0;
        }
        if (len > 0 && icon[len - 1] == 'n') {
            return 1;
        }
    }
    return 2;
}

static bool parse_weather_item(cJSON *weather, int *condition_id, uint8_t *icon_variant, char *description, size_t desc_len)
{
    if (!cJSON_IsArray(weather) || cJSON_GetArraySize(weather) <= 0) {
        return false;
    }
    cJSON *item = cJSON_GetArrayItem(weather, 0);
    if (!cJSON_IsObject(item)) {
        return false;
    }
    if (condition_id != NULL) {
        *condition_id = json_int_or(item, "id", *condition_id);
    }
    const char *icon = json_string_or(item, "icon");
    if (icon_variant != NULL) {
        *icon_variant = icon_variant_from_code(icon);
    }
    const char *desc = json_string_or(item, "description");
    if (description != NULL && desc_len > 0 && desc != NULL) {
        snprintf(description, desc_len, "%s", desc);
        description[desc_len - 1] = '\0';
    }
    return true;
}

static bool parse_current_json(const char *json, meteo_snapshot_t *snapshot)
{
    cJSON *root = cJSON_Parse(json);
    if (!cJSON_IsObject(root)) {
        if (root != NULL) {
            cJSON_Delete(root);
        }
        return false;
    }

    cJSON *main = obj_get(root, "main");
    float temp = NAN;
    if (cJSON_IsObject(main)) {
        temp = json_float_or(main, "temp", temp);
    }
    if (isnanf(temp)) {
        cJSON_Delete(root);
        return false;
    }

    snapshot->has_current = true;
    snapshot->current_temp_c = temp;
    cJSON *coord = obj_get(root, "coord");
    if (cJSON_IsObject(coord)) {
        cJSON *lat = obj_get(coord, "lat");
        cJSON *lon = obj_get(coord, "lon");
        if (cJSON_IsNumber(lat) && cJSON_IsNumber(lon)) {
            snapshot->has_coords = true;
            snapshot->lat = lat->valuedouble;
            snapshot->lon = lon->valuedouble;
        }
    }
    parse_weather_item(
        obj_get(root, "weather"),
        &snapshot->current_condition_id,
        &snapshot->current_icon_variant,
        snapshot->condition,
        sizeof(snapshot->condition));
    cJSON_Delete(root);
    return true;
}

static int find_day_slot(meteo_snapshot_t *snapshot, int year, int yday)
{
    int empty = -1;
    for (int i = 0; i < PULSEMON_METEO_FORECAST_DAYS; i++) {
        if (snapshot->days[i].valid && snapshot->days[i].year == year && snapshot->days[i].yday == yday) {
            return i;
        }
        if (!snapshot->days[i].valid && empty < 0) {
            empty = i;
        }
    }
    return empty;
}

static int local_day_index(time_t epoch, int16_t gmt_offset_min)
{
    if (epoch < PULSEMON_METEO_VALID_EPOCH_MIN) {
        return 0;
    }
    time_t adjusted = epoch + ((time_t)gmt_offset_min * 60);
    return (int)(adjusted / 86400);
}

static void forecast_day_update(meteo_day_t *day, const struct tm *tm, time_t timestamp, float min_c, float max_c, int condition_id, uint8_t variant)
{
    if (day == NULL || tm == NULL) {
        return;
    }
    uint8_t midday_offset = (uint8_t)abs(tm->tm_hour - 12);
    if (!day->valid) {
        day->valid = true;
        day->year = tm->tm_year;
        day->yday = tm->tm_yday;
        day->timestamp = timestamp;
        day->min_c = min_c;
        day->max_c = max_c;
        day->condition_id = condition_id;
        day->icon_variant = variant;
        day->midday_offset = midday_offset;
        return;
    }
    if (!isnanf(min_c) && (isnanf(day->min_c) || min_c < day->min_c)) {
        day->min_c = min_c;
    }
    if (!isnanf(max_c) && (isnanf(day->max_c) || max_c > day->max_c)) {
        day->max_c = max_c;
    }
    if (midday_offset < day->midday_offset) {
        day->timestamp = timestamp;
        day->condition_id = condition_id;
        day->icon_variant = variant;
        day->midday_offset = midday_offset;
    }
}

static bool parse_forecast_json(const char *json, int16_t gmt_offset_min, meteo_snapshot_t *snapshot)
{
    cJSON *root = cJSON_Parse(json);
    if (!cJSON_IsObject(root)) {
        if (root != NULL) {
            cJSON_Delete(root);
        }
        return false;
    }

    cJSON *list = obj_get(root, "list");
    if (!cJSON_IsArray(list)) {
        cJSON_Delete(root);
        return false;
    }

    time_t now = 0;
    time(&now);
    int today_index = local_day_index(now, gmt_offset_min);
    int filled = 0;
    int count = cJSON_GetArraySize(list);
    for (int i = 0; i < count; i++) {
        cJSON *item = cJSON_GetArrayItem(list, i);
        if (!cJSON_IsObject(item)) {
            continue;
        }
        time_t timestamp = (time_t)json_int_or(item, "dt", 0);
        struct tm local_tm;
        if (!local_tm_from_epoch(timestamp, gmt_offset_min, &local_tm)) {
            continue;
        }
        int day_offset = local_day_index(timestamp, gmt_offset_min) - today_index;
        if (day_offset < 1 || day_offset > PULSEMON_METEO_FORECAST_DAYS) {
            continue;
        }

        cJSON *main = obj_get(item, "main");
        float min_c = NAN;
        float max_c = NAN;
        if (cJSON_IsObject(main)) {
            min_c = json_float_or(main, "temp_min", min_c);
            max_c = json_float_or(main, "temp_max", max_c);
        }
        int condition_id = 0;
        uint8_t variant = 2;
        parse_weather_item(obj_get(item, "weather"), &condition_id, &variant, NULL, 0);

        int slot = find_day_slot(snapshot, local_tm.tm_year, local_tm.tm_yday);
        if (slot < 0) {
            break;
        }
        bool was_valid = snapshot->days[slot].valid;
        forecast_day_update(&snapshot->days[slot], &local_tm, timestamp, min_c, max_c, condition_id, variant);
        if (!was_valid && snapshot->days[slot].valid) {
            filled++;
        }
    }

    cJSON_Delete(root);
    return filled > 0;
}

static bool parse_onecall_daily_json(const char *json, int16_t gmt_offset_min, meteo_snapshot_t *snapshot)
{
    cJSON *root = cJSON_Parse(json);
    if (!cJSON_IsObject(root)) {
        if (root != NULL) {
            cJSON_Delete(root);
        }
        return false;
    }

    cJSON *daily = obj_get(root, "daily");
    if (!cJSON_IsArray(daily)) {
        cJSON_Delete(root);
        return false;
    }

    int filled = 0;
    int count = cJSON_GetArraySize(daily);
    for (int i = 1; i < count && i <= PULSEMON_METEO_FORECAST_DAYS; i++) {
        cJSON *item = cJSON_GetArrayItem(daily, i);
        if (!cJSON_IsObject(item)) {
            continue;
        }
        time_t timestamp = (time_t)json_int_or(item, "dt", 0);
        struct tm local_tm;
        if (!local_tm_from_epoch(timestamp, gmt_offset_min, &local_tm)) {
            continue;
        }

        cJSON *temp = obj_get(item, "temp");
        float min_c = NAN;
        float max_c = NAN;
        if (cJSON_IsObject(temp)) {
            min_c = json_float_or(temp, "min", min_c);
            max_c = json_float_or(temp, "max", max_c);
        }
        int condition_id = 0;
        uint8_t variant = 2;
        parse_weather_item(obj_get(item, "weather"), &condition_id, &variant, NULL, 0);

        int slot = find_day_slot(snapshot, local_tm.tm_year, local_tm.tm_yday);
        if (slot < 0) {
            break;
        }
        bool was_valid = snapshot->days[slot].valid;
        forecast_day_update(&snapshot->days[slot], &local_tm, timestamp, min_c, max_c, condition_id, variant);
        if (!was_valid && snapshot->days[slot].valid) {
            filled++;
        }
    }

    cJSON_Delete(root);
    return filled > 0;
}

static void apply_clock_ui(const pulsemon_settings_t *settings)
{
    time_t now = 0;
    time(&now);
    struct tm local_tm;
    if (!local_tm_from_epoch(now, settings->gmt_offset_min, &local_tm)) {
        return;
    }

    char time_buf[16];
    char date_buf[40];
    snprintf(time_buf, sizeof(time_buf), "%02d:%02d:%02d", local_tm.tm_hour, local_tm.tm_min, local_tm.tm_sec);
    snprintf(date_buf,
             sizeof(date_buf),
             "%s %d %s",
             weekday_long(settings->language, local_tm.tm_wday),
             local_tm.tm_mday,
             month_long(settings->language, local_tm.tm_mon));

    if (bsp_display_lock(pdMS_TO_TICKS(50))) {
        set_var_ui_meteo_houre(time_buf);
        set_var_ui_meteo_date(date_buf);
        bsp_display_unlock();
    }
}

static void apply_weather_ui(const meteo_snapshot_t *snapshot, const pulsemon_settings_t *settings)
{
    if (snapshot == NULL || settings == NULL) {
        return;
    }
    if (!bsp_display_lock(pdMS_TO_TICKS(100))) {
        METEO_LOGW("ui lock failed");
        return;
    }

    if (snapshot->has_current) {
        char temp_buf[16];
        snprintf(temp_buf, sizeof(temp_buf), "%.1f°C", (double)snapshot->current_temp_c);
        set_var_ui_meteo_temp(temp_buf);
        set_var_ui_meteo_condition(snapshot->condition[0] != '\0' ? snapshot->condition : "--");
        if (snapshot->current_condition_id != 0) {
            pulsemon_weather_icons_set_main((uint16_t)snapshot->current_condition_id, snapshot->current_icon_variant);
        }
    }

    for (int i = 0; i < PULSEMON_METEO_FORECAST_DAYS; i++) {
        char day_buf[8] = "--";
        char temp_buf[24] = "--";
        if (snapshot->days[i].valid) {
            struct tm day_tm;
            if (local_tm_from_epoch(snapshot->days[i].timestamp, settings->gmt_offset_min, &day_tm)) {
                snprintf(day_buf, sizeof(day_buf), "%s", weekday_short(settings->language, day_tm.tm_wday));
            }
            if (!isnanf(snapshot->days[i].min_c) && !isnanf(snapshot->days[i].max_c)) {
                snprintf(temp_buf, sizeof(temp_buf), "%.0f°/%.0f°", (double)snapshot->days[i].min_c, (double)snapshot->days[i].max_c);
            }
            if (snapshot->days[i].condition_id != 0) {
                lv_obj_t *icon = ui_weather_forecast_icon((size_t)i);
                if (icon != NULL) {
                    pulsemon_weather_icons_set_object(
                        icon,
                        "icon_50.bin",
                        (uint16_t)snapshot->days[i].condition_id,
                        snapshot->days[i].icon_variant);
                }
            }
        }

        switch (i) {
        case 0:
            set_var_ui_meteo_fd1(day_buf);
            set_var_ui_meteo_ft1(temp_buf);
            break;
        case 1:
            set_var_ui_meteo_fd2(day_buf);
            set_var_ui_meteo_ft2(temp_buf);
            break;
        case 2:
            set_var_ui_meteo_fd3(day_buf);
            set_var_ui_meteo_ft3(temp_buf);
            break;
        case 3:
            set_var_ui_meteo_fd4(day_buf);
            set_var_ui_meteo_ft4(temp_buf);
            break;
        case 4:
            set_var_ui_meteo_fd5(day_buf);
            set_var_ui_meteo_ft5(temp_buf);
            break;
        case 5:
            set_var_ui_meteo_fd6(day_buf);
            set_var_ui_meteo_ft6(temp_buf);
            break;
        default:
            break;
        }
    }
    bsp_display_unlock();
}

static void apply_config_missing_ui(const char *message)
{
    if (bsp_display_lock(pdMS_TO_TICKS(100))) {
        set_var_ui_meteo_temp("--");
        set_var_ui_meteo_condition(message ? message : "Config meteo");
        bsp_display_unlock();
    }
}

static void fetch_weather_once(void)
{
    pulsemon_settings_t settings;
    esp_err_t err = pulsemon_settings_load(&settings);
    if (err != ESP_OK && err != ESP_ERR_INVALID_SIZE) {
        METEO_LOGW("settings load failed: %s", esp_err_to_name(err));
        apply_config_missing_ui("Config meteo");
        return;
    }

    apply_clock_ui(&settings);

    if (settings.openweather_key[0] == '\0') {
        apply_config_missing_ui("Cle OpenWeather absente");
        return;
    }
    if (settings.openweather_city_id == 0) {
        apply_config_missing_ui("Ville absente");
        return;
    }
    if (!netif_ready()) {
        METEO_LOGW("network not ready");
        return;
    }

    char *body = (char *)calloc(1, PULSEMON_METEO_BODY_CAP);
    if (body == NULL) {
        METEO_LOGW("weather body alloc failed");
        return;
    }

    meteo_snapshot_t snapshot = {0};
    char url[320];
    snprintf(url,
             sizeof(url),
             "http://api.openweathermap.org/data/2.5/weather?id=%lu&appid=%s&units=metric&lang=%s",
             (unsigned long)settings.openweather_city_id,
             settings.openweather_key,
             settings.language);
    if (!fetch_url(url, body, PULSEMON_METEO_BODY_CAP) || !parse_current_json(body, &snapshot)) {
        METEO_LOGW("current weather update failed");
        if (s_has_last_snapshot) {
            apply_weather_ui(&s_last_snapshot, &settings);
        }
        free(body);
        return;
    }

    bool forecast_ok = false;
    if (snapshot.has_coords) {
        memset(body, 0, PULSEMON_METEO_BODY_CAP);
        snprintf(url,
                 sizeof(url),
                 "http://api.openweathermap.org/data/3.0/onecall?lat=%.6f&lon=%.6f&exclude=current,minutely,hourly,alerts&appid=%s&units=metric&lang=%s",
                 snapshot.lat,
                 snapshot.lon,
                 settings.openweather_key,
                 settings.language);
        forecast_ok = fetch_url(url, body, PULSEMON_METEO_BODY_CAP) && parse_onecall_daily_json(body, settings.gmt_offset_min, &snapshot);
    }
    if (!forecast_ok) {
        memset(body, 0, PULSEMON_METEO_BODY_CAP);
        snprintf(url,
                 sizeof(url),
                 "http://api.openweathermap.org/data/2.5/forecast?id=%lu&appid=%s&units=metric&lang=%s",
                 (unsigned long)settings.openweather_city_id,
                 settings.openweather_key,
                 settings.language);
        forecast_ok = fetch_url(url, body, PULSEMON_METEO_BODY_CAP) && parse_forecast_json(body, settings.gmt_offset_min, &snapshot);
    }
    if (!forecast_ok) {
        METEO_LOGW("forecast update failed");
    }

    s_last_snapshot = snapshot;
    s_has_last_snapshot = true;
    apply_weather_ui(&snapshot, &settings);
    free(body);
}

static void meteo_task(void *arg)
{
    (void)arg;
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        fetch_weather_once();
    }
}

static void clock_task(void *arg)
{
    (void)arg;
    while (1) {
        pulsemon_settings_t settings;
        esp_err_t err = pulsemon_settings_load(&settings);
        if (err == ESP_OK || err == ESP_ERR_INVALID_SIZE) {
            apply_clock_ui(&settings);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void meteo_timer_cb(void *arg)
{
    (void)arg;
    pulsemon_meteo_service_request_update();
}

static void start_sntp(void)
{
    if (esp_sntp_enabled()) {
        return;
    }
    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();
    METEO_LOGI("sntp started");
}

esp_err_t pulsemon_meteo_service_start(void)
{
    if (s_started) {
        return ESP_OK;
    }
    start_sntp();

    if (s_meteo_task == NULL) {
        BaseType_t ok = xTaskCreate(meteo_task, "MeteoTask", 8192, NULL, tskIDLE_PRIORITY + 2, &s_meteo_task);
        if (ok != pdPASS) {
            s_meteo_task = NULL;
            return ESP_ERR_NO_MEM;
        }
    }
    if (s_clock_task == NULL) {
        BaseType_t ok = xTaskCreate(clock_task, "MeteoClock", 3072, NULL, tskIDLE_PRIORITY + 1, &s_clock_task);
        if (ok != pdPASS) {
            s_clock_task = NULL;
            return ESP_ERR_NO_MEM;
        }
    }

    esp_timer_create_args_t timer_args = {
        .callback = meteo_timer_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "meteo_refresh",
        .skip_unhandled_events = true,
    };
    esp_err_t err = esp_timer_create(&timer_args, &s_meteo_timer);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_timer_start_periodic(
        s_meteo_timer,
        (int64_t)PULSEMON_METEO_REFRESH_MINUTES * 60 * 1000000LL);
    if (err != ESP_OK) {
        return err;
    }
    s_started = true;
    pulsemon_meteo_service_request_update();
    return ESP_OK;
}

void pulsemon_meteo_service_request_update(void)
{
    if (s_meteo_task != NULL) {
        xTaskNotifyGive(s_meteo_task);
    }
}
