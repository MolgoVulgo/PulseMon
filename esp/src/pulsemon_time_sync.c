#include "pulsemon_time_sync.h"

#include <time.h>

#include "esp_log.h"
#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define PULSEMON_TIME_VALID_EPOCH_MIN 1609459200L
#define PULSEMON_TIME_SYNC_POLL_MS 100U

static const char *TAG = "pulsemon_time";

bool pulsemon_time_sync_is_valid(void)
{
    time_t now = 0;
    time(&now);
    return now >= PULSEMON_TIME_VALID_EPOCH_MIN;
}

esp_err_t pulsemon_time_sync_start(void)
{
    if (esp_sntp_enabled()) {
        return ESP_OK;
    }

    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();
    ESP_LOGI(TAG, "SNTP started server=pool.ntp.org");
    return ESP_OK;
}

bool pulsemon_time_sync_wait(uint32_t timeout_ms)
{
    if (pulsemon_time_sync_is_valid()) {
        return true;
    }

    TickType_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);
    TickType_t poll_ticks = pdMS_TO_TICKS(PULSEMON_TIME_SYNC_POLL_MS);
    if (poll_ticks == 0) {
        poll_ticks = 1;
    }

    TickType_t started = xTaskGetTickCount();
    while ((xTaskGetTickCount() - started) < timeout_ticks) {
        vTaskDelay(poll_ticks);
        if (pulsemon_time_sync_is_valid()) {
            return true;
        }
    }

    return pulsemon_time_sync_is_valid();
}
