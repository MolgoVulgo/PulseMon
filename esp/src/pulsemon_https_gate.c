#include "pulsemon_https_gate.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/semphr.h"

#include "pulsemon_diag.h"

#ifndef PULSEMON_DEBUG
#define PULSEMON_DEBUG 0
#endif

static const char *TAG = "pulsemon_https";
static SemaphoreHandle_t s_https_mutex;

esp_err_t pulsemon_https_gate_init(void)
{
    if (s_https_mutex != NULL) {
        return ESP_OK;
    }

    s_https_mutex = xSemaphoreCreateMutex();
    if (s_https_mutex == NULL) {
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

bool pulsemon_https_gate_acquire(const char *owner, TickType_t wait_ticks)
{
    if (s_https_mutex == NULL) {
        ESP_LOGE(TAG, "gate unavailable owner=%s", owner != NULL ? owner : "?");
        return false;
    }

#if PULSEMON_DEBUG
    int64_t started_us = esp_timer_get_time();
    pulsemon_diag_heap(owner, "https_gate_wait");
#endif

    if (xSemaphoreTake(s_https_mutex, wait_ticks) != pdTRUE) {
        ESP_LOGW(TAG, "gate timeout owner=%s", owner != NULL ? owner : "?");
        return false;
    }

#if PULSEMON_DEBUG
    int64_t wait_ms = (esp_timer_get_time() - started_us) / 1000;
    ESP_LOGI(TAG, "gate acquired owner=%s wait_ms=%lld", owner != NULL ? owner : "?", (long long)wait_ms);
    pulsemon_diag_heap(owner, "https_gate_acquired");
#endif
    return true;
}

void pulsemon_https_gate_release(const char *owner)
{
    if (s_https_mutex == NULL) {
        return;
    }

#if PULSEMON_DEBUG
    pulsemon_diag_heap(owner, "https_gate_release");
#endif
    xSemaphoreGive(s_https_mutex);
}
