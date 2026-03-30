#include "pulsemon_poller.h"

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_timer.h"

#include "esp_bsp.h"
#include "pulsemon_api_client.h"
#include "pulsemon_api_config.h"
#include "ui_screen.h"
#include "vars.h"

static const char *TAG = "pulsemon_poller";

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
    snprintf(buf, sizeof(buf), "%.2f GiB", gib);
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

static void update_ui_from_dashboard(const pulsemon_dashboard_t *d)
{
    if (d == NULL) {
        return;
    }

    set_float_or_dash(set_var_cpu_pct, d->cpu_pct, d->cpu_pct_valid, "%.1f%%");
    set_float_or_dash(set_var_cpu_temp, d->cpu_temp_c, d->cpu_temp_c_valid, "%.1fC");
    set_float_or_dash(set_var_mem_pct, d->mem_pct, d->mem_pct_valid, "%.1f%%");
    set_bytes_gib_or_dash(set_var_mem_used, d->mem_used_b, d->mem_used_b_valid);
    set_bytes_gib_or_dash(set_var_mem_total, d->mem_total_b, d->mem_total_b_valid);
    set_float_or_dash(set_var_gpu_pct, d->gpu_pct, d->gpu_pct_valid, "%.1f%%");
    set_float_or_dash(set_var_gpu_temp, d->gpu_temp_c, d->gpu_temp_c_valid, "%.1fC");
    set_float_or_dash(set_var_gpu_power, d->gpu_power_w, d->gpu_power_w_valid, "%.0fW");

    char meta[128];
    const char *host = d->host_valid ? d->host : "host?";
    long stale = d->stale_ms_valid ? d->stale_ms : -1;
    const char *ok = d->state_ok_valid ? (d->state_ok ? "ok" : "degraded") : "state?";
    snprintf(meta, sizeof(meta), "%s | %s | stale=%ldms", host, ok, stale);
    meta[sizeof(meta) - 1] = '\0';
    set_var_host_meta(meta);
}

static void update_ui_from_gpu_dashboard(const pulsemon_gpu_dashboard_t *g)
{
    if (g == NULL) {
        return;
    }

    set_float_or_dash(set_var_gpu_pct, g->pct, g->pct_valid, "%.1f%%");
    set_float_or_dash(set_var_gpu_temp, g->temp_c, g->temp_c_valid, "%.1fC");
    set_float_or_dash(set_var_gpu_power, g->power_w, g->power_w_valid, "%.0fW");
    set_bytes_gib_or_dash(set_var_gpu_vram_total, g->vram_total_b, g->vram_total_b_valid);
    set_clock_or_dash(set_var_gpu_mem_clock, g->mem_clock_mhz, g->mem_clock_mhz_valid);

    if (g->fan_rpm_valid) {
        char fan_buf[32];
        if (g->fan_pct_valid) {
            snprintf(fan_buf, sizeof(fan_buf), "%.0f RPM (%.0f%%)", (double)g->fan_rpm, (double)g->fan_pct);
        } else {
            snprintf(fan_buf, sizeof(fan_buf), "%.0f RPM", (double)g->fan_rpm);
        }
        fan_buf[sizeof(fan_buf) - 1] = '\0';
        set_var_gpu_fan_rpm(fan_buf);
    } else {
        set_var_gpu_fan_rpm("--");
    }

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

static void mark_backend_offline(const char *why)
{
    set_var_host_meta(why ? why : "backend offline");
}

static void poller_task(void *arg)
{
    (void)arg;
    uint32_t tick_seq = 0;

    ESP_LOGI(TAG, "poller start target=%s:%d", PULSEMON_API_HOST, PULSEMON_API_PORT);

    while (1) {
        int64_t t_cycle_start_us = esp_timer_get_time();
        tick_seq++;

        bool gpu_page_active = (ui_screen_get_active() == SCREEN_ID_GPU);
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
        int64_t fetch_ms = (esp_timer_get_time() - t_cycle_start_us) / 1000;
        int64_t lock_wait_ms = -1;
        int64_t ui_apply_ms = -1;
        bool got_lock = false;
        bool ui_updated = false;

        if (!ok) {
            if (gpu_page_active) {
                ESP_LOGW(TAG, "gpu dashboard fetch failed: %s", gpu_err);
            } else {
                ESP_LOGW(TAG, "dashboard fetch failed: %s", err);
            }
            int64_t t_lock_wait_start_us = esp_timer_get_time();
            if (bsp_display_lock(100)) {
                lock_wait_ms = (esp_timer_get_time() - t_lock_wait_start_us) / 1000;
                got_lock = true;
                int64_t t_ui_apply_start_us = esp_timer_get_time();
                mark_backend_offline("backend offline");
                ui_apply_ms = (esp_timer_get_time() - t_ui_apply_start_us) / 1000;
                bsp_display_unlock();
            }
        } else {
            int64_t t_lock_wait_start_us = esp_timer_get_time();
            if (bsp_display_lock(100)) {
                lock_wait_ms = (esp_timer_get_time() - t_lock_wait_start_us) / 1000;
                got_lock = true;
                int64_t t_ui_apply_start_us = esp_timer_get_time();
                if (gpu_page_active) {
                    update_ui_from_gpu_dashboard(&gpu_dashboard);
                } else {
                    update_ui_from_dashboard(&dashboard);
                }
                ui_apply_ms = (esp_timer_get_time() - t_ui_apply_start_us) / 1000;
                ui_updated = true;
                bsp_display_unlock();
            }
        }

        int64_t cycle_ms = (esp_timer_get_time() - t_cycle_start_us) / 1000;
        LAT_DEBUG("tick=%lu screen=%s ok=%d fetch=%lldms lock=%lldms ui=%lldms cycle=%lldms stale=%ld",
                  (unsigned long)tick_seq,
                  gpu_page_active ? "gpu" : "main",
                  ok ? 1 : 0,
                  (long long)fetch_ms,
                  (long long)lock_wait_ms,
                  (long long)ui_apply_ms,
                  (long long)cycle_ms,
                  (!gpu_page_active && ok && dashboard.stale_ms_valid) ? dashboard.stale_ms : -1L);
        if (!got_lock) {
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

        vTaskDelay(pdMS_TO_TICKS(PULSEMON_DASHBOARD_POLL_MS));
    }
}

void pulsemon_poller_start(void)
{
    BaseType_t res = xTaskCreate(poller_task, "pulsemon_poller", 8192, NULL, 4, NULL);
    if (res != pdPASS) {
        ESP_LOGE(TAG, "failed to create poller task");
    }
}
