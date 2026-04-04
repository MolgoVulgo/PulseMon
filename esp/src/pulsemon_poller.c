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
    set_gpu_vram_used_total_or_dash(
        set_var_gpu_vram_total,
        g->vram_used_b,
        g->vram_used_b_valid,
        g->vram_total_b,
        g->vram_total_b_valid);
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

static void set_fan_slot_defaults(int slot_index)
{
    if (slot_index == 0) {
        set_var_fan_1_label("Fan 1");
        set_var_fan_1_rpm(0);
        set_var_fan_1_pct(0);
    } else if (slot_index == 1) {
        set_var_fan_2_label("Fan 2");
        set_var_fan_2_rpm(0);
        set_var_fan_2_pct(0);
    } else if (slot_index == 2) {
        set_var_fan_3_label("Fan 3");
        set_var_fan_3_rpm(0);
        set_var_fan_3_pct(0);
    }
}

static void update_ui_from_fans_dashboard(const pulsemon_fans_dashboard_t *f)
{
    if (f == NULL) {
        return;
    }

    for (int i = 0; i < PULSEMON_FAN_SLOT_COUNT; ++i) {
        const pulsemon_fan_slot_t *slot = &f->slots[i];
        if (!slot->has_data) {
            set_fan_slot_defaults(i);
            continue;
        }

        const char *label = slot->label_valid ? slot->label : (i == 0 ? "Fan 1" : (i == 1 ? "Fan 2" : "Fan 3"));
        int rpm = slot->rpm_valid ? slot->rpm : 0;
        int pct = slot->pct_valid ? slot->pct : 0;
        if (pct < 0) {
            pct = 0;
        } else if (pct > 100) {
            pct = 100;
        }

        if (i == 0) {
            set_var_fan_1_label(label);
            set_var_fan_1_rpm(rpm);
            set_var_fan_1_pct(pct);
        } else if (i == 1) {
            set_var_fan_2_label(label);
            set_var_fan_2_rpm(rpm);
            set_var_fan_2_pct(pct);
        } else if (i == 2) {
            set_var_fan_3_label(label);
            set_var_fan_3_rpm(rpm);
            set_var_fan_3_pct(pct);
        }
    }

    /* fan_1 always visible, fan_2 and fan_3 only when data exists. */
    ui_screen_set_fans_visibility(f->slots[1].has_data, f->slots[2].has_data);
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

        enum ScreensEnum active_screen = ui_screen_get_active();
        bool gpu_page_active = (active_screen == SCREEN_ID_GPU);
        bool fan_page_active = (active_screen == SCREEN_ID_FAN);
        bool ok = false;
        pulsemon_dashboard_t dashboard = {0};
        pulsemon_gpu_dashboard_t gpu_dashboard = {0};
        pulsemon_fans_dashboard_t fans_dashboard = {0};
        char err[64];
        char gpu_err[64];
        char fan_err[64];
        err[0] = '\0';
        gpu_err[0] = '\0';
        fan_err[0] = '\0';

        if (gpu_page_active) {
            ok = pulsemon_fetch_gpu_dashboard(&gpu_dashboard, gpu_err, sizeof(gpu_err));
        } else if (fan_page_active) {
            ok = pulsemon_fetch_fans_dashboard(&fans_dashboard, fan_err, sizeof(fan_err));
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
            } else if (fan_page_active) {
                ESP_LOGW(TAG, "fans dashboard fetch failed: %s", fan_err);
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
                } else if (fan_page_active) {
                    update_ui_from_fans_dashboard(&fans_dashboard);
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
                  gpu_page_active ? "gpu" : (fan_page_active ? "fan" : "main"),
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
