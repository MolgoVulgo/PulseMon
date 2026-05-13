#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "rom/ets_sys.h"

#include "esp_bsp.h"
#include "display.h"
#include "ui/ui.h"
#include "pulsemon_meteo_service.h"
#include "news_service.h"
#include "pulsemon_poller.h"
#include "pulsemon_weather_icons.h"
#include "ui_screen.h"
#include "vars.h"
#include "wifi_config_server.h"
#include "wifi_manager.h"
#include "wifi_captive_dns.h"
#include "lcd_capture.h"
#include "capture_config.h"

static const char *TAG = "pulsemon";

#ifndef PULSEMON_DEBUG
#define PULSEMON_DEBUG 0
#endif

static void pulsemon_on_wifi_connected(void)
{
    pulsemon_poller_start();
    pulsemon_meteo_service_request_update();
    news_service_request_update();
}

void app_main(void)
{
    ets_printf("pulsemon: app_main enter\n");
    ESP_LOGI(TAG, "app_main start");

    const bsp_display_cfg_t bsp_cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = EXAMPLE_LCD_QSPI_H_RES * EXAMPLE_LCD_QSPI_V_RES,
        .rotate = LV_DISP_ROT_270,
    };

    lv_disp_t *disp = bsp_display_start_with_config(&bsp_cfg);
    if (disp == NULL) {
        ESP_LOGE(TAG, "display init failed");
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    esp_err_t bl_ret = bsp_display_backlight_on();
    if (bl_ret != ESP_OK) {
        ESP_LOGE(TAG, "backlight on failed: %s", esp_err_to_name(bl_ret));
    }

    while (!bsp_display_lock(pdMS_TO_TICKS(1000))) {
        ESP_LOGW(TAG, "waiting lvgl lock...");
    }

    set_var_cpu_pct("--");
    set_var_cpu_temp("--");
    set_var_mem_pct("--");
    set_var_mem_used("--");
    set_var_mem_total("--");
    set_var_gpu_pct("--");
    set_var_gpu_temp("--");
    set_var_gpu_power("--");
    set_var_gpu_vram_total("--");
    set_var_gpu_mem_clock("--");
    set_var_gpu_fan_rpm("--");
    set_var_gpu_vram_used(0);
    set_var_fan_1_label("Fan 1");
    set_var_fan_1_rpm("0");
    set_var_fan_1_pct(0);
    set_var_fan_2_label("Fan 2");
    set_var_fan_2_rpm("0");
    set_var_fan_2_pct(0);
    set_var_fan_3_label("Fan 3");
    set_var_fan_3_rpm("0");
    set_var_fan_3_pct(0);
    set_var_ui_meteo_houre("--:--");
    set_var_ui_meteo_date("--");
    set_var_ui_meteo_temp("--");
    set_var_ui_meteo_condition("--");
    set_var_ui_meteo_alert("");
    set_var_ui_meteo_fd1("--");
    set_var_ui_meteo_fd2("--");
    set_var_ui_meteo_fd3("--");
    set_var_ui_meteo_fd4("--");
    set_var_ui_meteo_fd5("--");
    set_var_ui_meteo_fd6("--");
    set_var_ui_meteo_ft1("--");
    set_var_ui_meteo_ft2("--");
    set_var_ui_meteo_ft3("--");
    set_var_ui_meteo_ft4("--");
    set_var_ui_meteo_ft5("--");
    set_var_ui_meteo_ft6("--");
    set_var_host_meta("waiting backend");

    ui_init();
    ui_screen_start();

#if PULSEMON_SCREENSHOT_DEBUG
    const lcd_capture_cfg_t capture_cfg = {
        .width = EXAMPLE_LCD_QSPI_H_RES,
        .height = EXAMPLE_LCD_QSPI_V_RES,
        .interval_ms = PULSEMON_SCREENSHOT_INTERVAL_MS,
        .output_dir = PULSEMON_SCREENSHOT_DIR,
    };
    esp_err_t cap_ret = lcd_capture_start(&capture_cfg);
    if (cap_ret != ESP_OK) {
        ESP_LOGE(TAG, "lcd capture init failed: %s", esp_err_to_name(cap_ret));
    }
#endif

    bsp_display_unlock();

    ESP_LOGI(TAG, "ui started");

    esp_err_t icons_ret = pulsemon_weather_icons_init();
#if PULSEMON_DEBUG
    if (icons_ret != ESP_OK) {
        ESP_LOGW(TAG, "weather icons init skipped: %s", esp_err_to_name(icons_ret));
    }
#else
    (void)icons_ret;
#endif

    esp_err_t wifi_ret = pulsemon_wifi_manager_init(pulsemon_on_wifi_connected);
    if (wifi_ret != ESP_OK) {
        ESP_LOGE(TAG, "wifi manager init failed: %s", esp_err_to_name(wifi_ret));
    }

    if (wifi_ret == ESP_OK) {
        esp_err_t meteo_ret = pulsemon_meteo_service_start();
#if PULSEMON_DEBUG
        if (meteo_ret != ESP_OK) {
            ESP_LOGE(TAG, "meteo service init failed: %s", esp_err_to_name(meteo_ret));
        }
#else
        (void)meteo_ret;
#endif
        esp_err_t news_ret = news_service_start();
#if PULSEMON_NEWS_DEBUG
        if (news_ret != ESP_OK) {
            ESP_LOGE(TAG, "news service init failed: %s", esp_err_to_name(news_ret));
        }
#else
        (void)news_ret;
#endif
    }

    esp_err_t web_ret = pulsemon_wifi_config_server_start();
    if (web_ret != ESP_OK) {
        ESP_LOGE(TAG, "wifi config server init failed: %s", esp_err_to_name(web_ret));
    }

    esp_err_t dns_ret = pulsemon_wifi_captive_dns_start();
    if (dns_ret != ESP_OK) {
        ESP_LOGE(TAG, "wifi captive dns init failed: %s", esp_err_to_name(dns_ret));
    }

    if (wifi_ret == ESP_OK) {
        wifi_ret = pulsemon_wifi_manager_start();
        if (wifi_ret != ESP_OK) {
            ESP_LOGE(TAG, "wifi manager start failed: %s", esp_err_to_name(wifi_ret));
        }
    }

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
