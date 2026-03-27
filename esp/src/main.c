#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "esp_bsp.h"
#include "ui/ui.h"
#include "ui_screen.h"
#include "vars.h"

static const char *TAG = "pulsemon";

void app_main(void)
{
    const bsp_display_cfg_t bsp_cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = 320 * 40,
        .rotate = LV_DISP_ROT_NONE,
    };

    lv_disp_t *disp = bsp_display_start_with_config(&bsp_cfg);
    if (disp == NULL) {
        ESP_LOGE(TAG, "display init failed");
        return;
    }

    if (!bsp_display_lock(0)) {
        ESP_LOGE(TAG, "lvgl lock failed");
        return;
    }

    set_var_cpu_pct("--");
    set_var_cpu_temp("--");
    set_var_mem_pct("--");
    set_var_mem_used("--");
    set_var_mem_total("--");
    set_var_gpu_pct("--");
    set_var_gpu_temp("--");
    set_var_gpu_power("--");
    set_var_host_meta("waiting backend");

    ui_init();
    ui_screen_start();

    bsp_display_unlock();

    ESP_LOGI(TAG, "ui started");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
