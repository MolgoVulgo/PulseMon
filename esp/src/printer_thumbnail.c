#include "printer_thumbnail.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "esp_bsp.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#define PRINTER_THUMBNAIL_CACHE_SLOTS 1
#define PRINTER_THUMBNAIL_LOCK_MS 250

static const char *TAG = "printer_thumb_ui";

static lv_obj_t *s_frame;
static lv_obj_t *s_image;
static lv_img_dsc_t s_desc[2];
static uint8_t *s_owned_png[2];
static unsigned s_active_desc;
static bool s_has_image;

static uint16_t thumbnail_zoom(uint16_t width, uint16_t height)
{
    if (s_frame == NULL || width == 0 || height == 0) {
        return 256;
    }

    int32_t frame_w = lv_obj_get_content_width(s_frame);
    int32_t frame_h = lv_obj_get_content_height(s_frame);
    if (frame_w <= 0 || frame_h <= 0) {
        return 256;
    }

    uint32_t zoom_w = ((uint32_t)frame_w * 256U) / width;
    uint32_t zoom_h = ((uint32_t)frame_h * 256U) / height;
    uint32_t zoom = zoom_w < zoom_h ? zoom_w : zoom_h;
    if (zoom > 256U) {
        zoom = 256U;
    }
    if (zoom == 0U) {
        zoom = 1U;
    }
    return (uint16_t)zoom;
}

void printer_thumbnail_init(lv_obj_t *frame)
{
    s_frame = frame;
    if (s_frame == NULL) {
        ESP_LOGE(TAG, "thumbnail init failed: image_gode frame is NULL");
        return;
    }
    if (s_image != NULL) {
        ESP_LOGI(TAG,
                 "thumbnail init already done frame=%p image=%p",
                 (void *)s_frame,
                 (void *)s_image);
        return;
    }

    lv_img_cache_set_size(PRINTER_THUMBNAIL_CACHE_SLOTS);

    s_image = lv_img_create(s_frame);
    if (s_image == NULL) {
        ESP_LOGE(TAG, "thumbnail lv_img_create failed frame=%p", (void *)s_frame);
        return;
    }
    lv_obj_clear_flag(s_image, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_image, LV_OBJ_FLAG_HIDDEN);

    ESP_LOGI(TAG,
             "thumbnail UI initialized frame=%p frame_size=%dx%d content=%dx%d image=%p png_decoder=%d",
             (void *)s_frame,
             (int)lv_obj_get_width(s_frame),
             (int)lv_obj_get_height(s_frame),
             (int)lv_obj_get_content_width(s_frame),
             (int)lv_obj_get_content_height(s_frame),
             (void *)s_image,
#if LV_USE_PNG
             1
#else
             0
#endif
    );
#if !LV_USE_PNG
    ESP_LOGE(TAG, "thumbnail PNG decoder is disabled in LVGL configuration");
#endif
}

esp_err_t printer_thumbnail_set_png(uint8_t *png_data, size_t png_len, uint16_t width, uint16_t height)
{
    if (png_data == NULL || png_len == 0 || width == 0 || height == 0 || png_len > UINT32_MAX) {
        ESP_LOGE(TAG,
                 "thumbnail set rejected data=%p bytes=%u size=%ux%u",
                 (void *)png_data,
                 (unsigned)png_len,
                 (unsigned)width,
                 (unsigned)height);
        free(png_data);
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG,
             "thumbnail set begin png=%p bytes=%u size=%ux%u",
             (void *)png_data,
             (unsigned)png_len,
             (unsigned)width,
             (unsigned)height);

    if (!bsp_display_lock(pdMS_TO_TICKS(PRINTER_THUMBNAIL_LOCK_MS))) {
        ESP_LOGW(TAG,
                 "thumbnail set display lock timeout after=%u ms",
                 (unsigned)PRINTER_THUMBNAIL_LOCK_MS);
        free(png_data);
        return ESP_ERR_TIMEOUT;
    }

    if (s_image == NULL) {
        ESP_LOGE(TAG, "thumbnail set failed: UI image is not initialized");
        bsp_display_unlock();
        free(png_data);
        return ESP_ERR_INVALID_STATE;
    }

    unsigned next_desc = s_has_image ? (s_active_desc ^ 1U) : 0U;
    if (s_owned_png[next_desc] != NULL) {
        lv_img_cache_invalidate_src(&s_desc[next_desc]);
        free(s_owned_png[next_desc]);
        s_owned_png[next_desc] = NULL;
    }

    memset(&s_desc[next_desc], 0, sizeof(s_desc[next_desc]));
    s_desc[next_desc].header.cf = LV_IMG_CF_RAW_ALPHA;
    s_desc[next_desc].header.w = width;
    s_desc[next_desc].header.h = height;
    s_desc[next_desc].data_size = (uint32_t)png_len;
    s_desc[next_desc].data = png_data;

    lv_img_set_src(s_image, &s_desc[next_desc]);
    lv_obj_set_size(s_image, width, height);
    lv_img_set_pivot(s_image, width / 2U, height / 2U);
    uint16_t zoom = thumbnail_zoom(width, height);
    lv_img_set_zoom(s_image, zoom);
    lv_obj_center(s_image);
    lv_obj_clear_flag(s_image, LV_OBJ_FLAG_HIDDEN);

    if (s_has_image) {
        lv_img_cache_invalidate_src(&s_desc[s_active_desc]);
        free(s_owned_png[s_active_desc]);
        s_owned_png[s_active_desc] = NULL;
        memset(&s_desc[s_active_desc], 0, sizeof(s_desc[s_active_desc]));
    }

    s_owned_png[next_desc] = png_data;
    s_active_desc = next_desc;
    s_has_image = true;

    ESP_LOGI(TAG,
             "thumbnail set complete desc=%u frame=%dx%d content=%dx%d image=%ux%u zoom=%u",
             next_desc,
             (int)lv_obj_get_width(s_frame),
             (int)lv_obj_get_height(s_frame),
             (int)lv_obj_get_content_width(s_frame),
             (int)lv_obj_get_content_height(s_frame),
             (unsigned)width,
             (unsigned)height,
             (unsigned)zoom);

    bsp_display_unlock();
    return ESP_OK;
}

esp_err_t printer_thumbnail_clear(void)
{
    if (!bsp_display_lock(pdMS_TO_TICKS(PRINTER_THUMBNAIL_LOCK_MS))) {
        ESP_LOGW(TAG,
                 "thumbnail clear display lock timeout after=%u ms",
                 (unsigned)PRINTER_THUMBNAIL_LOCK_MS);
        return ESP_ERR_TIMEOUT;
    }

    bool had_cached_image = s_has_image;

    if (s_image != NULL) {
        lv_obj_add_flag(s_image, LV_OBJ_FLAG_HIDDEN);
    }

    if (s_has_image) {
        lv_img_cache_invalidate_src(&s_desc[s_active_desc]);
        free(s_owned_png[s_active_desc]);
        s_owned_png[s_active_desc] = NULL;
        memset(&s_desc[s_active_desc], 0, sizeof(s_desc[s_active_desc]));
        s_has_image = false;
    }

    ESP_LOGI(TAG,
             "thumbnail cleared image=%p had_cached_image=%d",
             (void *)s_image,
             had_cached_image ? 1 : 0);

    bsp_display_unlock();
    return ESP_OK;
}
