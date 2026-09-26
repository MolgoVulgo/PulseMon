#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
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
#include "pulsemon_time_sync.h"
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

#define PULSEMON_STARTUP_WIFI_WAIT_MS 10000U
#define PULSEMON_STARTUP_NTP_WAIT_MS 8000U
#define PULSEMON_STARTUP_METEO_WAIT_MS 12000U
#define PULSEMON_STARTUP_NEWS_WAIT_MS 10000U
#define PULSEMON_STARTUP_BACKEND_WAIT_MS 9000U

#define PULSEMON_STARTUP_EVENT_WIFI_CONNECTED BIT0
#define PULSEMON_STARTUP_EVENT_ABORT BIT1

static EventGroupHandle_t s_startup_events;
static TaskHandle_t s_network_start_task;
static bool s_startup_screen_released;
static bool s_network_sequence_started;

static void startup_progress(int32_t pct, const char *text)
{
    if (s_startup_screen_released) {
        return;
    }
    if (bsp_display_lock(pdMS_TO_TICKS(1000))) {
        ui_screen_set_start_progress(pct, text);
        bsp_display_unlock();
    } else {
        ESP_LOGW(TAG, "startup progress lock timeout");
    }
}

static void startup_release_screen(enum ScreensEnum target_screen, bool pc_screens_enabled, const char *text)
{
    if (s_startup_screen_released) {
        return;
    }

    startup_progress(100, text);
    vTaskDelay(pdMS_TO_TICKS(250));
    if (bsp_display_lock(pdMS_TO_TICKS(1000))) {
        ui_screen_set_pc_screens_enabled(pc_screens_enabled);
        ui_screen_show_and_release_start(target_screen);
        s_startup_screen_released = true;
        bsp_display_unlock();
    } else {
        ESP_LOGW(TAG, "startup target screen lock timeout");
    }
}

static void pulsemon_network_start_fallback(void)
{
    ESP_LOGW(TAG, "startup sequencer unavailable; launching network services without waits");
    (void)pulsemon_time_sync_start();
    pulsemon_meteo_service_request_update();
    news_service_request_update();
    printer_service_request_update();
    pulsemon_poller_start();
}

static void pulsemon_network_start_task(void *arg)
{
    (void)arg;

    EventBits_t bits = xEventGroupWaitBits(s_startup_events,
                                           PULSEMON_STARTUP_EVENT_WIFI_CONNECTED | PULSEMON_STARTUP_EVENT_ABORT,
                                           pdFALSE,
                                           pdFALSE,
                                           pdMS_TO_TICKS(PULSEMON_STARTUP_WIFI_WAIT_MS));
    if ((bits & PULSEMON_STARTUP_EVENT_ABORT) != 0) {
        startup_release_screen(SCREEN_ID_METEO, false, "Pret: Wi-Fi indisponible");
        pulsemon_poller_set_ui_ready(true);
        s_network_start_task = NULL;
        vTaskDelete(NULL);
        return;
    }

    if ((bits & PULSEMON_STARTUP_EVENT_WIFI_CONNECTED) == 0) {
        ESP_LOGW(TAG, "startup Wi-Fi wait timeout; releasing startup screen to meteo and waiting in background");
        startup_release_screen(SCREEN_ID_METEO, false, "Pret: hors ligne");
        pulsemon_poller_set_ui_ready(true);
        bits = xEventGroupWaitBits(s_startup_events,
                                   PULSEMON_STARTUP_EVENT_WIFI_CONNECTED | PULSEMON_STARTUP_EVENT_ABORT,
                                   pdFALSE,
                                   pdFALSE,
                                   portMAX_DELAY);
        if ((bits & PULSEMON_STARTUP_EVENT_ABORT) != 0) {
            s_network_start_task = NULL;
            vTaskDelete(NULL);
            return;
        }
    }

    s_network_sequence_started = true;
    pulsemon_diag_heap("startup", "wifi_connected");
    startup_progress(45, "Wi-Fi: connecte");

    startup_progress(55, "Heure: NTP");
    esp_err_t time_ret = pulsemon_time_sync_start();
    bool time_ok = time_ret == ESP_OK && pulsemon_time_sync_wait(PULSEMON_STARTUP_NTP_WAIT_MS);
    if (time_ok) {
        ESP_LOGI(TAG, "startup NTP synchronized");
    } else {
        ESP_LOGW(TAG, "startup NTP not synchronized within %u ms", (unsigned)PULSEMON_STARTUP_NTP_WAIT_MS);
    }

    startup_progress(65, "Meteo");
    pulsemon_diag_heap("startup", "first_meteo");
    if (!pulsemon_meteo_service_request_update_and_wait(PULSEMON_STARTUP_METEO_WAIT_MS)) {
        ESP_LOGW(TAG, "startup weather cycle still pending after %u ms", (unsigned)PULSEMON_STARTUP_METEO_WAIT_MS);
    }

    startup_progress(75, "Actualites");
    pulsemon_diag_heap("startup", "first_news");
    if (!news_service_request_update_and_wait(PULSEMON_STARTUP_NEWS_WAIT_MS)) {
        ESP_LOGW(TAG, "startup news cycle still pending after %u ms", (unsigned)PULSEMON_STARTUP_NEWS_WAIT_MS);
    }

    startup_progress(85, "Imprimante");
    pulsemon_diag_heap("startup", "first_printer");
    printer_service_request_update();

    startup_progress(95, "Stats PC");
    pulsemon_diag_heap("startup", "start_poller");
    pulsemon_poller_start();

    bool backend_resolved = pulsemon_poller_wait_initial_state(PULSEMON_STARTUP_BACKEND_WAIT_MS);
    pulsemon_backend_state_t backend_state = pulsemon_poller_get_backend_state();
    bool backend_online = backend_state == PULSEMON_BACKEND_ONLINE;
    if (!backend_resolved) {
        ESP_LOGW(TAG,
                 "startup backend state unresolved after %u ms (state=%d); using autonomous screens",
                 (unsigned)PULSEMON_STARTUP_BACKEND_WAIT_MS,
                 (int)backend_state);
    }

    const char *ready_text = backend_online ? (time_ok ? "Pret" : "Pret: heure en attente")
                                             : "Pret: PC hors ligne";
    startup_release_screen(backend_online ? SCREEN_ID_MAIN : SCREEN_ID_METEO,
                           backend_online,
                           ready_text);
    pulsemon_poller_set_ui_ready(true);
    s_network_start_task = NULL;
    vTaskDelete(NULL);
}

static esp_err_t pulsemon_network_start_init(void)
{
    if (s_startup_events == NULL) {
        s_startup_events = xEventGroupCreate();
        if (s_startup_events == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }
    if (s_network_start_task != NULL) {
        return ESP_OK;
    }

    BaseType_t ok = xTaskCreate(pulsemon_network_start_task,
                                "startup_net",
                                4096,
                                NULL,
                                tskIDLE_PRIORITY + 2,
                                &s_network_start_task);
    if (ok != pdPASS) {
        s_network_start_task = NULL;
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

static void pulsemon_on_wifi_connected(void)
{
    if (s_network_start_task != NULL && s_startup_events != NULL) {
        xEventGroupSetBits(s_startup_events, PULSEMON_STARTUP_EVENT_WIFI_CONNECTED);
        return;
    }

    if (!s_network_sequence_started) {
        s_network_sequence_started = true;
        pulsemon_network_start_fallback();
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

    startup_progress(25, "Meteo: icones");
    esp_err_t icons_ret = pulsemon_weather_icons_init();
#if PULSEMON_DEBUG
    if (icons_ret != ESP_OK) {
        ESP_LOGW(TAG, "weather icons init skipped: %s", esp_err_to_name(icons_ret));
    }
#else
    (void)icons_ret;
#endif

    startup_progress(30, "Reseau: init");
    esp_err_t diag_ret = pulsemon_diag_init();
    if (diag_ret != ESP_OK) {
        ESP_LOGW(TAG, "memory diagnostics init failed: %s", esp_err_to_name(diag_ret));
    }
    pulsemon_diag_heap("startup", "before_network_init");

    esp_err_t https_gate_ret = pulsemon_https_gate_init();
    if (https_gate_ret != ESP_OK) {
        ESP_LOGE(TAG, "https gate init failed: %s", esp_err_to_name(https_gate_ret));
    }

    esp_err_t wifi_ret = pulsemon_wifi_manager_init(pulsemon_on_wifi_connected, pulsemon_on_config_mode_changed);
    if (wifi_ret != ESP_OK) {
        ESP_LOGE(TAG, "wifi manager init failed: %s", esp_err_to_name(wifi_ret));
    }

    if (wifi_ret == ESP_OK) {
        startup_progress(35, "Meteo: service");
        esp_err_t meteo_ret = pulsemon_meteo_service_start();
#if PULSEMON_DEBUG
        if (meteo_ret != ESP_OK) {
            ESP_LOGE(TAG, "meteo service init failed: %s", esp_err_to_name(meteo_ret));
        }
#else
        (void)meteo_ret;
#endif
        startup_progress(38, "News: service");
        esp_err_t news_ret = news_service_start();
#if PULSEMON_NEWS_DEBUG
        if (news_ret != ESP_OK) {
            ESP_LOGE(TAG, "news service init failed: %s", esp_err_to_name(news_ret));
        }
#else
        (void)news_ret;
#endif
    } else {
        startup_progress(38, "Services reseau indisponibles");
    }

    bool network_start_ready = false;
    if (wifi_ret == ESP_OK) {
        esp_err_t startup_net_ret = pulsemon_network_start_init();
        if (startup_net_ret != ESP_OK) {
            ESP_LOGE(TAG, "startup network sequencer init failed: %s", esp_err_to_name(startup_net_ret));
        } else {
            network_start_ready = true;
        }

        startup_progress(40, "Wi-Fi: connexion");
        wifi_ret = pulsemon_wifi_manager_start();
        if (wifi_ret != ESP_OK) {
            ESP_LOGE(TAG, "wifi manager start failed: %s", esp_err_to_name(wifi_ret));
            if (network_start_ready && s_startup_events != NULL) {
                xEventGroupSetBits(s_startup_events, PULSEMON_STARTUP_EVENT_ABORT);
            }
        } else {
            esp_err_t trigger_ret = pulsemon_config_mode_trigger_start();
            if (trigger_ret != ESP_OK) {
                ESP_LOGE(TAG, "local config trigger init failed: %s", esp_err_to_name(trigger_ret));
            }
        }
    }

    if (!network_start_ready) {
        startup_release_screen(SCREEN_ID_METEO,
                               false,
                               wifi_ret == ESP_OK ? "Pret: backend en attente" : "Pret: Wi-Fi indisponible");
        pulsemon_poller_set_ui_ready(true);
    }

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
