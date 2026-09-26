#include "pulsemon_poller.h"

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_timer.h"

#include "esp_bsp.h"
#include "pulsemon_api_client.h"
#include "pulsemon_api_config.h"
#include "pulsemon_api_settings.h"
#include "pulsemon_diag.h"
#include "ui_screen.h"
#include "vars.h"

static const char *TAG = "pulsemon_poller";
static TaskHandle_t s_poller_task;
static EventGroupHandle_t s_backend_events;
static volatile pulsemon_backend_state_t s_backend_state = PULSEMON_BACKEND_UNKNOWN;
static volatile bool s_ui_ready;
static bool s_auto_switched_to_meteo;
static int64_t s_failure_started_us;
static uint32_t s_recovery_successes;

#define PULSEMON_BACKEND_EVENT_RESOLVED BIT0

#ifndef PULSEMON_LATENCY_DEBUG
#define PULSEMON_LATENCY_DEBUG 0
#endif

#ifndef PULSEMON_UI_WARN_MS
#define PULSEMON_UI_WARN_MS 120
#endif

#if PULSEMON_LATENCY_DEBUG
#define LAT_DEBUG(fmt, ...) ESP_LOGI(TAG, fmt, ##__VA_ARGS__)
#else
#define LAT_DEBUG(fmt, ...) ((void)0)
#endif

static void set_float_or_dash(void (*setter)(const char *), float value, bool valid, const char *fmt)
{
    if (setter == NULL) {
        return;
    }
    if (!valid) {
        setter("--");
        return;
    }
    char buf[32];
    snprintf(buf, sizeof(buf), fmt, value);
    buf[sizeof(buf) - 1] = '\0';
    setter(buf);
}

static void set_bytes_gib_or_dash(void (*setter)(const char *), unsigned long long value, bool valid)
{
    if (setter == NULL) {
        return;
    }
    if (!valid) {
        setter("--");
        return;
    }
    char buf[32];
    double gib = (double)value / (1024.0 * 1024.0 * 1024.0);
    snprintf(buf, sizeof(buf), "%.2f", gib);
    buf[sizeof(buf) - 1] = '\0';
    setter(buf);
}

static void set_clock_or_dash(void (*setter)(const char *), float mhz, bool valid)
{
    if (setter == NULL) {
        return;
    }
    if (!valid) {
        setter("--");
        return;
    }
    char buf[32];
    if (mhz >= 1000.0f) {
        snprintf(buf, sizeof(buf), "%.2f GHz", (double)(mhz / 1000.0f));
    } else {
        snprintf(buf, sizeof(buf), "%.0f MHz", (double)mhz);
    }
    buf[sizeof(buf) - 1] = '\0';
    setter(buf);
}

static void set_gpu_vram_used_total_or_dash(
    void (*setter)(const char *),
    unsigned long long used_b,
    bool used_valid,
    unsigned long long total_b,
    bool total_valid)
{
    if (setter == NULL) {
        return;
    }
    if (!used_valid || !total_valid) {
        setter("--");
        return;
    }

    const double gib_div = 1024.0 * 1024.0 * 1024.0;
    double used_go = (double)used_b / gib_div;
    double total_go = (double)total_b / gib_div;
    char buf[32];
    snprintf(buf, sizeof(buf), "%.0f / %.0f Go", used_go, total_go);
    buf[sizeof(buf) - 1] = '\0';
    setter(buf);
}

static void update_ui_from_dashboard(const pulsemon_dashboard_t *d)
{
    if (d == NULL) {
        return;
    }

    set_float_or_dash(set_var_cpu_pct, d->cpu_pct, d->cpu_pct_valid, "%.1f");
    set_float_or_dash(set_var_cpu_temp, d->cpu_temp_c, d->cpu_temp_c_valid, "%.1f");
    set_float_or_dash(set_var_mem_pct, d->mem_pct, d->mem_pct_valid, "%.1f");
    set_bytes_gib_or_dash(set_var_mem_used, d->mem_used_b, d->mem_used_b_valid);
    set_bytes_gib_or_dash(set_var_mem_total, d->mem_total_b, d->mem_total_b_valid);
    set_float_or_dash(set_var_gpu_pct, d->gpu_pct, d->gpu_pct_valid, "%.1f");
    set_float_or_dash(set_var_gpu_temp, d->gpu_temp_c, d->gpu_temp_c_valid, "%.1f");
    set_float_or_dash(set_var_gpu_power, d->gpu_power_w, d->gpu_power_w_valid, "%.0f");

    char meta[128];
    const char *host = d->host_valid ? d->host : "host?";
    const char *ok = d->state_ok_valid ? (d->state_ok ? "ok" : "degraded") : "state?";
    snprintf(meta, sizeof(meta), "%s | %s", host, ok);
    meta[sizeof(meta) - 1] = '\0';
    set_var_host_meta(meta);
}

static void update_ui_from_gpu_dashboard(const pulsemon_gpu_dashboard_t *g)
{
    if (g == NULL) {
        return;
    }

    set_float_or_dash(set_var_gpu_pct, g->pct, g->pct_valid, "%.1f");
    set_float_or_dash(set_var_gpu_temp, g->temp_c, g->temp_c_valid, "%.1f");
    set_float_or_dash(set_var_gpu_power, g->power_w, g->power_w_valid, "%.0f");
    set_gpu_vram_used_total_or_dash(
        set_var_gpu_vram_total,
        g->vram_used_b,
        g->vram_used_b_valid,
        g->vram_total_b,
        g->vram_total_b_valid);
    set_clock_or_dash(set_var_gpu_mem_clock, g->mem_clock_mhz, g->mem_clock_mhz_valid);

    if (g->fan_rpm_valid) {
        char fan_buf[32];
        snprintf(fan_buf, sizeof(fan_buf), "%.0f", (double)g->fan_rpm);
        fan_buf[sizeof(fan_buf) - 1] = '\0';
        set_var_gpu_fan_rpm(fan_buf);
    } else {
        set_var_gpu_fan_rpm("--");
    }
    set_float_or_dash(set_var_gpu_fan_rpm_1, g->fan_pct, g->fan_pct_valid, "%.0f");

    if (g->vram_pct_valid) {
        int vram_pct = (int)(g->vram_pct + 0.5f);
        if (vram_pct < 0) {
            vram_pct = 0;
        }
        if (vram_pct > 100) {
            vram_pct = 100;
        }
        set_var_gpu_vram_used(vram_pct);
    } else {
        set_var_gpu_vram_used(0);
    }
}

static const char *backend_state_name(pulsemon_backend_state_t state)
{
    switch (state) {
    case PULSEMON_BACKEND_UNKNOWN:
        return "unknown";
    case PULSEMON_BACKEND_ONLINE:
        return "online";
    case PULSEMON_BACKEND_SUSPECT:
        return "suspect";
    case PULSEMON_BACKEND_OFFLINE:
        return "offline";
    default:
        return "invalid";
    }
}

static void mark_backend_offline(const char *why)
{
    set_var_host_meta(why ? why : "backend offline");
}

static void apply_backend_offline_ui(bool startup_reconcile)
{
    if (!s_ui_ready) {
        return;
    }

    if (!bsp_display_lock(pdMS_TO_TICKS(1000))) {
        ESP_LOGW(TAG, "backend offline UI lock timeout");
        return;
    }

    ui_screen_set_pc_screens_enabled(false);
    enum ScreensEnum active_screen = ui_screen_get_active();
    mark_backend_offline("backend offline");

    if (active_screen == SCREEN_ID_MAIN || active_screen == SCREEN_ID_GPU) {
        ui_screen_load(SCREEN_ID_METEO, LV_SCR_LOAD_ANIM_MOVE_LEFT);
        s_auto_switched_to_meteo = true;
        ESP_LOGI(TAG, "backend offline: PC screens locked, auto switch to meteo");
    } else if (startup_reconcile && active_screen == SCREEN_ID_METEO) {
        s_auto_switched_to_meteo = true;
    }

    bsp_display_unlock();
}

static void apply_backend_online_ui(void)
{
    if (!s_ui_ready) {
        return;
    }

    if (!bsp_display_lock(pdMS_TO_TICKS(1000))) {
        ESP_LOGW(TAG, "backend online UI lock timeout");
        return;
    }

    ui_screen_set_pc_screens_enabled(true);
    enum ScreensEnum active_screen = ui_screen_get_active();
    if (s_auto_switched_to_meteo) {
        if (active_screen == SCREEN_ID_METEO) {
            ui_screen_load(SCREEN_ID_MAIN, LV_SCR_LOAD_ANIM_MOVE_RIGHT);
            ESP_LOGI(TAG, "backend online: PC screens unlocked, auto switch to main");
        }
        s_auto_switched_to_meteo = false;
    }

    bsp_display_unlock();
}

static void set_backend_state(pulsemon_backend_state_t next)
{
    pulsemon_backend_state_t previous = s_backend_state;
    if (previous == next) {
        return;
    }

    s_backend_state = next;
    ESP_LOGI(TAG, "backend state %s -> %s", backend_state_name(previous), backend_state_name(next));

    if ((next == PULSEMON_BACKEND_ONLINE || next == PULSEMON_BACKEND_OFFLINE) && s_backend_events != NULL) {
        xEventGroupSetBits(s_backend_events, PULSEMON_BACKEND_EVENT_RESOLVED);
    }

    if (next == PULSEMON_BACKEND_OFFLINE) {
        apply_backend_offline_ui(false);
    } else if (next == PULSEMON_BACKEND_ONLINE) {
        apply_backend_online_ui();
    }
}

static void backend_record_failure(void)
{
    int64_t now_us = esp_timer_get_time();

    switch (s_backend_state) {
    case PULSEMON_BACKEND_UNKNOWN:
    case PULSEMON_BACKEND_ONLINE:
        s_failure_started_us = now_us;
        s_recovery_successes = 0;
        set_backend_state(PULSEMON_BACKEND_SUSPECT);
        break;

    case PULSEMON_BACKEND_SUSPECT:
        if (s_failure_started_us <= 0) {
            s_failure_started_us = now_us;
        }
        if ((now_us - s_failure_started_us) >= ((int64_t)PULSEMON_BACKEND_OFFLINE_GRACE_MS * 1000LL)) {
            s_recovery_successes = 0;
            set_backend_state(PULSEMON_BACKEND_OFFLINE);
        }
        break;

    case PULSEMON_BACKEND_OFFLINE:
        s_recovery_successes = 0;
        break;

    default:
        break;
    }
}

static void backend_record_success(void)
{
    switch (s_backend_state) {
    case PULSEMON_BACKEND_UNKNOWN:
    case PULSEMON_BACKEND_SUSPECT:
        s_failure_started_us = 0;
        s_recovery_successes = 0;
        set_backend_state(PULSEMON_BACKEND_ONLINE);
        break;

    case PULSEMON_BACKEND_OFFLINE:
        s_recovery_successes++;
        ESP_LOGI(TAG,
                 "backend recovery success %lu/%u",
                 (unsigned long)s_recovery_successes,
                 (unsigned)PULSEMON_BACKEND_RECOVERY_SUCCESSES);
        if (s_recovery_successes >= PULSEMON_BACKEND_RECOVERY_SUCCESSES) {
            s_failure_started_us = 0;
            s_recovery_successes = 0;
            set_backend_state(PULSEMON_BACKEND_ONLINE);
        }
        break;

    case PULSEMON_BACKEND_ONLINE:
        s_failure_started_us = 0;
        s_recovery_successes = 0;
        break;

    default:
        break;
    }
}

static void poller_task(void *arg)
{
    (void)arg;
    uint32_t tick_seq = 0;

    esp_err_t endpoint_err = pulsemon_api_client_reload_endpoint();
    if (endpoint_err != ESP_OK && endpoint_err != ESP_ERR_INVALID_SIZE) {
        ESP_LOGW(TAG, "backend endpoint load failed: %s", esp_err_to_name(endpoint_err));
    }
    pulsemon_api_settings_t endpoint;
    pulsemon_api_client_get_endpoint(&endpoint);
    ESP_LOGI(TAG, "poller start target=%s:%u", endpoint.host, (unsigned)endpoint.port);

    while (1) {
#if PULSEMON_LATENCY_DEBUG
        int64_t t_cycle_start_us = esp_timer_get_time();
#endif
        tick_seq++;

        enum ScreensEnum active_screen = ui_screen_get_active();
        bool gpu_page_active = (active_screen == SCREEN_ID_GPU);
        bool ok = false;
        pulsemon_dashboard_t dashboard = {0};
        pulsemon_gpu_dashboard_t gpu_dashboard = {0};
        char err[64];
        char gpu_err[64];
        err[0] = '\0';
        gpu_err[0] = '\0';

        if (gpu_page_active) {
            ok = pulsemon_fetch_gpu_dashboard(&gpu_dashboard, gpu_err, sizeof(gpu_err));
        } else {
            ok = pulsemon_fetch_dashboard(&dashboard, err, sizeof(err));
        }
#if PULSEMON_LATENCY_DEBUG
        int64_t fetch_ms = (esp_timer_get_time() - t_cycle_start_us) / 1000;
        int64_t lock_wait_ms = -1;
        int64_t ui_apply_ms = -1;
        bool lock_attempted = false;
        bool got_lock = false;
        bool ui_updated = false;
#endif

        if (!ok) {
            if (gpu_page_active) {
                ESP_LOGW(TAG, "gpu dashboard fetch failed: %s", gpu_err);
            } else {
                ESP_LOGW(TAG, "dashboard fetch failed: %s", err);
            }
            backend_record_failure();
        } else {
            backend_record_success();

            /* A single successful probe while OFFLINE is only recovery evidence.
             * Keep the offline presentation until the configured consecutive-success
             * threshold has been reached. */
            if (s_backend_state == PULSEMON_BACKEND_ONLINE) {
#if PULSEMON_LATENCY_DEBUG
                lock_attempted = true;
                int64_t t_lock_wait_start_us = esp_timer_get_time();
#endif
                if (bsp_display_lock(100)) {
#if PULSEMON_LATENCY_DEBUG
                    lock_wait_ms = (esp_timer_get_time() - t_lock_wait_start_us) / 1000;
                    got_lock = true;
                    int64_t t_ui_apply_start_us = esp_timer_get_time();
#endif
                    if (gpu_page_active) {
                        update_ui_from_gpu_dashboard(&gpu_dashboard);
                    } else {
                        update_ui_from_dashboard(&dashboard);
                    }
#if PULSEMON_LATENCY_DEBUG
                    ui_apply_ms = (esp_timer_get_time() - t_ui_apply_start_us) / 1000;
                    ui_updated = true;
#endif
                    bsp_display_unlock();
                }
            }
        }

#if PULSEMON_LATENCY_DEBUG
        int64_t cycle_ms = (esp_timer_get_time() - t_cycle_start_us) / 1000;
        LAT_DEBUG("tick=%lu screen=%s ok=%d state=%s fetch=%lldms lock=%lldms ui=%lldms cycle=%lldms",
                  (unsigned long)tick_seq,
                  gpu_page_active ? "gpu" :
                      (active_screen == SCREEN_ID_METEO ? "meteo" :
                           (active_screen == SCREEN_ID_PRINTER ? "printer" : "main")),
                  ok ? 1 : 0,
                  backend_state_name(s_backend_state),
                  (long long)fetch_ms,
                  (long long)lock_wait_ms,
                  (long long)ui_apply_ms,
                  (long long)cycle_ms);
        if (lock_attempted && !got_lock) {
            ESP_LOGW(TAG, "ui lock timeout tick=%lu", (unsigned long)tick_seq);
        }
        if (got_lock && lock_wait_ms > PULSEMON_UI_WARN_MS) {
            ESP_LOGW(TAG, "ui lock wait high=%lldms tick=%lu", (long long)lock_wait_ms, (unsigned long)tick_seq);
        }
        if (ui_updated && ui_apply_ms > PULSEMON_UI_WARN_MS) {
            ESP_LOGW(TAG, "ui apply high=%lldms tick=%lu", (long long)ui_apply_ms, (unsigned long)tick_seq);
        }
        if (cycle_ms > PULSEMON_DASHBOARD_POLL_MS) {
            ESP_LOGW(TAG, "poll cycle overrun=%lldms tick=%lu", (long long)cycle_ms, (unsigned long)tick_seq);
        }
#endif

        if (tick_seq == 1U || (tick_seq % 30U) == 0U) {
            pulsemon_diag_heap("poller", "periodic");
            pulsemon_diag_stack("poller", "periodic");
        }

        vTaskDelay(pdMS_TO_TICKS(PULSEMON_DASHBOARD_POLL_MS));
    }
}

void pulsemon_poller_start(void)
{
    if (s_poller_task != NULL) {
        return;
    }

    if (s_backend_events == NULL) {
        s_backend_events = xEventGroupCreate();
        if (s_backend_events == NULL) {
            ESP_LOGW(TAG, "backend state event group unavailable");
        }
    }

    BaseType_t res = xTaskCreate(poller_task, "pulsemon_poller", 6144, NULL, 4, &s_poller_task);
    if (res != pdPASS) {
        s_poller_task = NULL;
        ESP_LOGE(TAG, "failed to create poller task");
        set_backend_state(PULSEMON_BACKEND_OFFLINE);
    }
}

pulsemon_backend_state_t pulsemon_poller_get_backend_state(void)
{
    return s_backend_state;
}

bool pulsemon_poller_wait_initial_state(uint32_t timeout_ms)
{
    pulsemon_backend_state_t state = s_backend_state;
    if (state == PULSEMON_BACKEND_ONLINE || state == PULSEMON_BACKEND_OFFLINE) {
        return true;
    }
    if (s_backend_events == NULL) {
        return false;
    }

    EventBits_t bits = xEventGroupWaitBits(s_backend_events,
                                           PULSEMON_BACKEND_EVENT_RESOLVED,
                                           pdFALSE,
                                           pdFALSE,
                                           pdMS_TO_TICKS(timeout_ms));
    state = s_backend_state;
    return (bits & PULSEMON_BACKEND_EVENT_RESOLVED) != 0 ||
           state == PULSEMON_BACKEND_ONLINE || state == PULSEMON_BACKEND_OFFLINE;
}

void pulsemon_poller_set_ui_ready(bool ready)
{
    s_ui_ready = ready;
    if (!ready) {
        return;
    }

    if (s_backend_state == PULSEMON_BACKEND_ONLINE) {
        apply_backend_online_ui();
    } else {
        apply_backend_offline_ui(true);
    }
}
