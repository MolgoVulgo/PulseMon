#include "pulsemon_poller.h"

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "esp_bsp.h"
#include "pulsemon_api_client.h"
#include "pulsemon_api_config.h"
#include "vars.h"

static const char *TAG = "pulsemon_poller";

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

static void mark_backend_offline(const char *why)
{
    set_var_host_meta(why ? why : "backend offline");
}

static void poller_task(void *arg)
{
    (void)arg;

    ESP_LOGI(TAG, "poller start target=%s:%d", PULSEMON_API_HOST, PULSEMON_API_PORT);

    while (1) {
        pulsemon_dashboard_t dashboard;
        char err[64];
        bool ok = pulsemon_fetch_dashboard(&dashboard, err, sizeof(err));
        if (!ok) {
            ESP_LOGW(TAG, "dashboard fetch failed: %s", err);
            if (bsp_display_lock(100)) {
                mark_backend_offline("backend offline");
                bsp_display_unlock();
            }
        } else {
            if (bsp_display_lock(100)) {
                update_ui_from_dashboard(&dashboard);
                bsp_display_unlock();
            }
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
