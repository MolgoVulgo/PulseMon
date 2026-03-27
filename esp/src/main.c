#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "rom/ets_sys.h"
#include "nvs_flash.h"

#include "esp_bsp.h"
#include "display.h"
#include "ui/ui.h"
#include "pulsemon_poller.h"
#include "ui_screen.h"
#include "vars.h"
#include "wifi_config.h"

static const char *TAG = "pulsemon";
static EventGroupHandle_t s_wifi_event_group;
static esp_event_handler_instance_t s_wifi_handler_any_id;
static esp_event_handler_instance_t s_wifi_handler_got_ip;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define WIFI_MAX_RETRY 10

static int s_wifi_retry_count = 0;

static void pulsemon_wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    (void)arg;
    (void)event_data;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        return;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_wifi_retry_count < WIFI_MAX_RETRY) {
            s_wifi_retry_count++;
            ESP_LOGW(TAG, "wifi disconnected, retry %d/%d", s_wifi_retry_count, WIFI_MAX_RETRY);
            esp_wifi_connect();
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        return;
    }

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        s_wifi_retry_count = 0;
        ESP_LOGI(TAG, "wifi connected ip=" IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void pulsemon_network_stack_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "nvs init failed (%s), erasing", esp_err_to_name(ret));
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ret = esp_netif_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(ret);
    }

    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(ret);
    }
}

static void pulsemon_wifi_connect(void)
{
    s_wifi_event_group = xEventGroupCreate();
    if (s_wifi_event_group == NULL) {
        ESP_LOGE(TAG, "wifi event group alloc failed");
        return;
    }

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &pulsemon_wifi_event_handler, NULL, &s_wifi_handler_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &pulsemon_wifi_event_handler, NULL, &s_wifi_handler_got_ip));

    wifi_config_t wifi_config = {0};
    snprintf((char *)wifi_config.sta.ssid, sizeof(wifi_config.sta.ssid), "%s", WIFI_SSID);
    snprintf((char *)wifi_config.sta.password, sizeof(wifi_config.sta.password), "%s", WIFI_PASSWORD);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        pdMS_TO_TICKS(20000));

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "wifi ready (ssid=%s)", WIFI_SSID);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "wifi failed after retries (ssid=%s)", WIFI_SSID);
    } else {
        ESP_LOGE(TAG, "wifi connect timeout (ssid=%s)", WIFI_SSID);
    }
}

void app_main(void)
{
    ets_printf("pulsemon: app_main enter\n");
    ESP_LOGI(TAG, "app_main start");

    const bsp_display_cfg_t bsp_cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = EXAMPLE_LCD_QSPI_H_RES * EXAMPLE_LCD_QSPI_V_RES,
        .rotate = LV_DISP_ROT_90,
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
    set_var_host_meta("waiting backend");

    ui_init();
    ui_screen_start();

    bsp_display_unlock();

    ESP_LOGI(TAG, "ui started");

    pulsemon_network_stack_init();
    pulsemon_wifi_connect();
    pulsemon_poller_start();

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
