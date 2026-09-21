#include "config_mode_trigger.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_bsp.h"
#include "esp_log.h"
#include "lvgl.h"

#include "ui/screens.h"
#include "wifi_config.h"
#include "wifi_manager.h"

static const char *TAG = "config_trigger";

static bool s_started;
static bool s_hold_active;
static bool s_hold_fired;
static uint32_t s_hold_started_ms;

static void open_config_mode_task(void *arg)
{
    (void)arg;

    esp_err_t err = pulsemon_wifi_manager_open_config_mode();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "local configuration mode requested");
    } else {
        ESP_LOGE(TAG, "local configuration mode request failed: %s", esp_err_to_name(err));
    }

    vTaskDelete(NULL);
}

static void reset_hold_state(void)
{
    s_hold_active = false;
    s_hold_fired = false;
    s_hold_started_ms = 0;
}

static void hotspot_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);

    if (code == LV_EVENT_PRESSED) {
        s_hold_started_ms = lv_tick_get();
        s_hold_active = true;
        s_hold_fired = false;
        return;
    }

    if (code == LV_EVENT_PRESSING && s_hold_active && !s_hold_fired) {
        if (lv_tick_elaps(s_hold_started_ms) < PULSEMON_WIFI_LOCAL_TRIGGER_HOLD_MS) {
            return;
        }

        s_hold_fired = true;
        BaseType_t created = xTaskCreate(
            open_config_mode_task,
            "config_mode",
            3072,
            NULL,
            4,
            NULL);
        if (created != pdPASS) {
            ESP_LOGE(TAG, "unable to create configuration mode task");
            return;
        }

        lv_indev_t *indev = lv_event_get_indev(event);
        if (indev != NULL) {
            lv_indev_wait_release(indev);
        }
        return;
    }

    if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        reset_hold_state();
    }
}

static esp_err_t add_hotspot(lv_obj_t *screen)
{
    if (screen == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    lv_obj_t *hotspot = lv_obj_create(screen);
    if (hotspot == NULL) {
        return ESP_ERR_NO_MEM;
    }

    lv_obj_remove_style_all(hotspot);
    lv_obj_set_size(
        hotspot,
        PULSEMON_WIFI_LOCAL_TRIGGER_HOTSPOT_PX,
        PULSEMON_WIFI_LOCAL_TRIGGER_HOTSPOT_PX);
    lv_obj_align(hotspot, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_add_flag(hotspot, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(hotspot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(hotspot, hotspot_event_cb, LV_EVENT_ALL, NULL);
    return ESP_OK;
}

esp_err_t pulsemon_config_mode_trigger_start(void)
{
    if (s_started) {
        return ESP_OK;
    }

    if (!bsp_display_lock(1000)) {
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t err = add_hotspot(objects.main);
    if (err == ESP_OK) {
        err = add_hotspot(objects.gpu);
    }
    if (err == ESP_OK) {
        err = add_hotspot(objects.meteo);
    }

    bsp_display_unlock();

    if (err == ESP_OK) {
        reset_hold_state();
        s_started = true;
        ESP_LOGI(
            TAG,
            "local trigger ready: hold top-left corner for %u ms",
            (unsigned)PULSEMON_WIFI_LOCAL_TRIGGER_HOLD_MS);
    }
    return err;
}
