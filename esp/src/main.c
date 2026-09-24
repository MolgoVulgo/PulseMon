#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "rom/ets_sys.h"

#include <stdint.h>

#include "esp_bsp.h"
#include "display.h"
#include "ui/ui.h"
#include "pulsemon_meteo_service.h"
#include "news_service.h"
#include "pulsemon_poller.h"
#include "pulsemon_diag.h"
#include "pulsemon_https_gate.h"
#include "printer_service.h"
#include "pulsemon_weather_icons.h"
#include "ui_screen.h"
#include "vars.h"
#include "wifi_config_server.h"
#include "wifi_manager.h"
#include "wifi_captive_dns.h"
#include "lcd_capture.h"
#include "capture_config.h"
#include "config_mode_trigger.h"

static const char *TAG = "pulsemon";

#ifndef PULSEMON_DEBUG
#define PULSEMON_DEBUG 0
#endif

#define PULSEMON_START_POLLER_DELAY_US 500000LL
#define PULSEMON_START_METEO_DELAY_US 1500000LL
#define PULSEMON_START_NEWS_DELAY_US 6000000LL
#define PULSEMON_START_PRINTER_DELAY_US 10000000LL

typedef enum {
    PULSEMON_NETWORK_START_IDLE = 0,
    PULSEMON_NETWORK_START_POLLER,
    PULSEMON_NETWORK_START_METEO,
    PULSEMON_NETWORK_START_NEWS,
    PULSEMON_NETWORK_START_PRINTER,
} pulsemon_network_start_step_t;

static esp_timer_handle_t s_network_start_timer;
static pulsemon_network_start_step_t s_network_start_step;

static void startup_progress(int32_t pct, const char *text)
{
    if (bsp_display_lock(pdMS_TO_TICKS(1000))) {
        ui_screen_set_start_progress(pct, text);
        bsp_display_unlock();
    } else {
        ESP_LOGW(TAG, "startup progress lock timeout");
    }
}

static void pulsemon_network_start_timer_cb(void *arg)
{
    (void)arg;

    switch (s_network_start_step) {
    case PULSEMON_NETWORK_START_POLLER:
        pulsemon_diag_heap("startup", "stagger_poller");
        ESP_LOGI(TAG, "network stagger: backend poller");
        pulsemon_poller_start();
        s_network_start_step = PULSEMON_NETWORK_START_METEO;
        if (esp_timer_start_once(s_network_start_timer, PULSEMON_START_METEO_DELAY_US) != ESP_OK) {
            ESP_LOGE(TAG, "network stagger: failed to schedule meteo");
            pulsemon_meteo_service_request_update();
            s_network_start_step = PULSEMON_NETWORK_START_IDLE;
        }
        break;

    case PULSEMON_NETWORK_START_METEO:
        pulsemon_diag_heap("startup", "stagger_meteo");
        ESP_LOGI(TAG, "network stagger: meteo");
        pulsemon_meteo_service_request_update();
        s_network_start_step = PULSEMON_NETWORK_START_NEWS;
        if (esp_timer_start_once(s_network_start_timer, PULSEMON_START_NEWS_DELAY_US) != ESP_OK) {
            ESP_LOGE(TAG, "network stagger: failed to schedule news; periodic refresh will retry");
            s_network_start_step = PULSEMON_NETWORK_START_IDLE;
        }
        break;

    case PULSEMON_NETWORK_START_NEWS:
        pulsemon_diag_heap("startup", "stagger_news");
        ESP_LOGI(TAG, "network stagger: news");
        news_service_request_update();
        s_network_start_step = PULSEMON_NETWORK_START_PRINTER;
        if (esp_timer_start_once(s_network_start_timer, PULSEMON_START_PRINTER_DELAY_US) != ESP_OK) {
            ESP_LOGE(TAG, "network stagger: failed to schedule printer probe; periodic refresh will retry");
            s_network_start_step = PULSEMON_NETWORK_START_IDLE;
        }
        break;

    case PULSEMON_NETWORK_START_PRINTER:
        pulsemon_diag_heap("startup", "stagger_printer");
        ESP_LOGI(TAG, "network stagger: printer presence");
        printer_service_request_update();
        s_network_start_step = PULSEMON_NETWORK_START_IDLE;
        break;

    case PULSEMON_NETWORK_START_IDLE:
    default:
        break;
    }
}

static esp_err_t pulsemon_network_start_timer_init(void)
{
    if (s_network_start_timer != NULL) {
        return ESP_OK;
    }

    const esp_timer_create_args_t timer_args = {
        .callback = pulsemon_network_start_timer_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "network_stagger",
        .skip_unhandled_events = true,
    };
    return esp_timer_create(&timer_args, &s_network_start_timer);
}

static void pulsemon_on_wifi_connected(void)
{
    pulsemon_diag_heap("startup", "wifi_connected");

    if (s_network_start_timer == NULL) {
        ESP_LOGE(TAG, "network stagger unavailable; starting backend and meteo, news deferred to periodic refresh");
        pulsemon_poller_start();
        pulsemon_meteo_service_request_update();
        printer_service_request_update();
        return;
    }

    esp_err_t stop_err = esp_timer_stop(s_network_start_timer);
    if (stop_err != ESP_OK && stop_err != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "network stagger stop failed: %s", esp_err_to_name(stop_err));
    }

    s_network_start_step = PULSEMON_NETWORK_START_POLLER;
    esp_err_t start_err = esp_timer_start_once(s_network_start_timer, PULSEMON_START_POLLER_DELAY_US);
    if (start_err != ESP_OK) {
        ESP_LOGE(TAG, "network stagger start failed: %s", esp_err_to_name(start_err));
        s_network_start_step = PULSEMON_NETWORK_START_IDLE;
        pulsemon_poller_start();
        pulsemon_meteo_service_request_update();
        printer_service_request_update();
    }
}

static void pulsemon_on_config_mode_changed(bool active)
{
    if (active) {
        esp_err_t web_ret = pulsemon_wifi_config_server_start();
        if (web_ret != ESP_OK) {
            ESP_LOGE(TAG, "wifi config server start failed: %s", esp_err_to_name(web_ret));
            return;
        }

        esp_err_t dns_ret = pulsemon_wifi_captive_dns_start();
        if (dns_ret != ESP_OK) {
            ESP_LOGE(TAG, "wifi captive dns start failed: %s", esp_err_to_name(dns_ret));
        }
        return;
    }

    esp_err_t dns_ret = pulsemon_wifi_captive_dns_stop();
    if (dns_ret != ESP_OK) {
        ESP_LOGE(TAG, "wifi captive dns stop failed: %s", esp_err_to_name(dns_ret));
    }

    esp_err_t web_ret = pulsemon_wifi_config_server_stop();
    if (web_ret != ESP_OK) {
        ESP_LOGE(TAG, "wifi config server stop failed: %s", esp_err_to_name(web_ret));
    }
}

void app_main(void)
{
#if PULSEMON_DEBUG
    ets_printf("pulsemon: app_main enter\n");
#endif
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
    set_var_gpu_fan_rpm_1("--");
    set_var_gpu_vram_used(0);
    set_var_name_printer("--");
    set_var_printer_ip("--");
    set_var_print_file_name("--");
    set_var_print_time_start("--:--");
    set_var_print_time_end("--:--");
    set_var_print_time_elapsed("00:00:00");
    set_var_print_time_remaining("00:00:00");
    set_var_print_layer("--/--");
    set_var_print_bar(0);
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
    set_var_ui_start_bar(0);
    set_var_ui_start_bar_texte("Demarrage");
    set_var_host_meta("waiting backend");

    esp_err_t printer_ret = printer_service_init();
    if (printer_ret != ESP_OK) {
        ESP_LOGE(TAG, "printer service init failed: %s", esp_err_to_name(printer_ret));
    }

    ui_init();
    ui_screen_set_start_progress(10, "UI");
    ui_screen_start();
    ui_screen_set_start_progress(20, "Mire prete");

#if PULSEMON_SCREENSHOT_DEBUG && PULSEMON_SCREENSHOT_AUTOSTART
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

    startup_progress(30, "Meteo: icones");
    esp_err_t icons_ret = pulsemon_weather_icons_init();
#if PULSEMON_DEBUG
    if (icons_ret != ESP_OK) {
        ESP_LOGW(TAG, "weather icons init skipped: %s", esp_err_to_name(icons_ret));
    }
#else
    (void)icons_ret;
#endif

    startup_progress(40, "Reseau: init");
    esp_err_t diag_ret = pulsemon_diag_init();
    if (diag_ret != ESP_OK) {
        ESP_LOGW(TAG, "memory diagnostics init failed: %s", esp_err_to_name(diag_ret));
    }
    pulsemon_diag_heap("startup", "before_network_init");

    esp_err_t https_gate_ret = pulsemon_https_gate_init();
    if (https_gate_ret != ESP_OK) {
        ESP_LOGE(TAG, "https gate init failed: %s", esp_err_to_name(https_gate_ret));
    }

    esp_err_t stagger_ret = pulsemon_network_start_timer_init();
    if (stagger_ret != ESP_OK) {
        ESP_LOGE(TAG, "network stagger timer init failed: %s", esp_err_to_name(stagger_ret));
    }

    esp_err_t wifi_ret = pulsemon_wifi_manager_init(pulsemon_on_wifi_connected, pulsemon_on_config_mode_changed);
    if (wifi_ret != ESP_OK) {
        ESP_LOGE(TAG, "wifi manager init failed: %s", esp_err_to_name(wifi_ret));
    }

    if (wifi_ret == ESP_OK) {
        startup_progress(55, "Meteo: service");
        esp_err_t meteo_ret = pulsemon_meteo_service_start();
#if PULSEMON_DEBUG
        if (meteo_ret != ESP_OK) {
            ESP_LOGE(TAG, "meteo service init failed: %s", esp_err_to_name(meteo_ret));
        }
#else
        (void)meteo_ret;
#endif
        startup_progress(70, "News: service");
        esp_err_t news_ret = news_service_start();
#if PULSEMON_NEWS_DEBUG
        if (news_ret != ESP_OK) {
            ESP_LOGE(TAG, "news service init failed: %s", esp_err_to_name(news_ret));
        }
#else
        (void)news_ret;
#endif
    } else {
        startup_progress(70, "Services reseau indisponibles");
    }

    if (wifi_ret == ESP_OK) {
        startup_progress(85, "Monitoring: Wi-Fi");
        wifi_ret = pulsemon_wifi_manager_start();
        if (wifi_ret != ESP_OK) {
            ESP_LOGE(TAG, "wifi manager start failed: %s", esp_err_to_name(wifi_ret));
        } else {
            esp_err_t trigger_ret = pulsemon_config_mode_trigger_start();
            if (trigger_ret != ESP_OK) {
                ESP_LOGE(TAG, "local config trigger init failed: %s", esp_err_to_name(trigger_ret));
            }
        }
    }

    startup_progress(100, wifi_ret == ESP_OK ? "Monitoring: pret" : "Monitoring: Wi-Fi indisponible");
    vTaskDelay(pdMS_TO_TICKS(250));
    if (bsp_display_lock(pdMS_TO_TICKS(1000))) {
        ui_screen_show_main_and_release_start();
        bsp_display_unlock();
    } else {
        ESP_LOGW(TAG, "startup main screen lock timeout");
    }

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
