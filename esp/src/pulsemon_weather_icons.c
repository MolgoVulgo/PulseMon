#include "pulsemon_weather_icons.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"

#include "capture_config.h"
#include "sd_storage.h"
#include "svg2bin_decoder.h"
#include "ui_backend.h"

#define WEATHER_ICON_BIN_MAX 24
#define WEATHER_ICON_CACHE_MAX 12
#define WEATHER_ICON_SLOT_MAX 8

#ifndef PULSEMON_DEBUG
#define PULSEMON_DEBUG 0
#endif

#if PULSEMON_DEBUG
static const char *TAG = "weather_icons";
#define WEATHER_ICON_LOGI(fmt, ...) ESP_LOGI(TAG, fmt, ##__VA_ARGS__)
#define WEATHER_ICON_LOGW(fmt, ...) ESP_LOGW(TAG, fmt, ##__VA_ARGS__)
#else
#define WEATHER_ICON_LOGI(fmt, ...) ((void)0)
#define WEATHER_ICON_LOGW(fmt, ...) ((void)0)
#endif

typedef struct {
    lv_img_dsc_t dsc;
    uint8_t *data;
    lv_obj_t *target;
    uint16_t last_code;
    uint8_t last_variant;
    bool has_last;
    char last_bin[WEATHER_ICON_BIN_MAX];
} weather_icon_ctx_t;

typedef struct {
    uint16_t code;
    uint8_t variant;
    uint16_t width;
    uint16_t height;
    size_t data_size;
    uint8_t *data;
    uint32_t last_use;
    char bin[WEATHER_ICON_BIN_MAX];
} weather_icon_cache_t;

static weather_icon_ctx_t s_icon_slots[WEATHER_ICON_SLOT_MAX];
static weather_icon_cache_t s_icon_cache[WEATHER_ICON_CACHE_MAX];
static uint32_t s_icon_cache_use_counter;

static void icon_ctx_reset(weather_icon_ctx_t *ctx, lv_obj_t *target)
{
    if (ctx == NULL) {
        return;
    }
    if (ctx->data != NULL) {
        free(ctx->data);
        ctx->data = NULL;
    }
    memset(&ctx->dsc, 0, sizeof(ctx->dsc));
    ctx->target = target;
    ctx->has_last = false;
    ctx->last_code = 0;
    ctx->last_variant = 0;
    ctx->last_bin[0] = '\0';
}

static weather_icon_ctx_t *icon_ctx_for_target(lv_obj_t *target)
{
    if (target == NULL) {
        return NULL;
    }
    for (size_t i = 0; i < WEATHER_ICON_SLOT_MAX; i++) {
        if (s_icon_slots[i].target == target) {
            return &s_icon_slots[i];
        }
    }
    for (size_t i = 0; i < WEATHER_ICON_SLOT_MAX; i++) {
        if (s_icon_slots[i].target == NULL) {
            icon_ctx_reset(&s_icon_slots[i], target);
            return &s_icon_slots[i];
        }
    }
    icon_ctx_reset(&s_icon_slots[0], target);
    return &s_icon_slots[0];
}

static void rgb565_swap(uint8_t *buf, size_t len)
{
    for (size_t i = 0; i + 1 < len; i += 2) {
        uint8_t tmp = buf[i];
        buf[i] = buf[i + 1];
        buf[i + 1] = tmp;
    }
}

static weather_icon_cache_t *icon_cache_find(const char *bin, uint16_t code, uint8_t variant)
{
    for (size_t i = 0; i < WEATHER_ICON_CACHE_MAX; i++) {
        weather_icon_cache_t *entry = &s_icon_cache[i];
        if (entry->data == NULL) {
            continue;
        }
        if (entry->code == code && entry->variant == variant && strcmp(entry->bin, bin) == 0) {
            entry->last_use = ++s_icon_cache_use_counter;
            return entry;
        }
    }
    return NULL;
}

static weather_icon_cache_t *icon_cache_reserve(const char *bin, uint16_t code, uint8_t variant)
{
    weather_icon_cache_t *oldest = NULL;
    for (size_t i = 0; i < WEATHER_ICON_CACHE_MAX; i++) {
        weather_icon_cache_t *entry = &s_icon_cache[i];
        if (entry->data == NULL) {
            entry->code = code;
            entry->variant = variant;
            snprintf(entry->bin, sizeof(entry->bin), "%s", bin);
            entry->last_use = ++s_icon_cache_use_counter;
            return entry;
        }
        if (oldest == NULL || entry->last_use < oldest->last_use) {
            oldest = entry;
        }
    }
    if (oldest != NULL) {
        free(oldest->data);
        oldest->data = NULL;
        oldest->data_size = 0;
        oldest->code = code;
        oldest->variant = variant;
        snprintf(oldest->bin, sizeof(oldest->bin), "%s", bin);
        oldest->last_use = ++s_icon_cache_use_counter;
    }
    return oldest;
}

static esp_err_t icon_apply_cached(weather_icon_ctx_t *ctx, const weather_icon_cache_t *cache)
{
    if (ctx == NULL || cache == NULL || cache->data == NULL || ctx->target == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t *copy = (uint8_t *)malloc(cache->data_size);
    if (copy == NULL) {
        return ESP_ERR_NO_MEM;
    }
    memcpy(copy, cache->data, cache->data_size);
    if (ctx->data != NULL) {
        free(ctx->data);
    }
    ctx->data = copy;
    memset(&ctx->dsc, 0, sizeof(ctx->dsc));
    ctx->dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
    ctx->dsc.header.w = cache->width;
    ctx->dsc.header.h = cache->height;
    ctx->dsc.data_size = cache->data_size;
    ctx->dsc.data = ctx->data;
    lv_img_set_src(ctx->target, &ctx->dsc);
    return ESP_OK;
}

static esp_err_t icon_draw_cb(void *user_ctx, const char *name, uint16_t width, uint16_t height, const uint8_t *rgb565, size_t rgb565_len)
{
    (void)name;
    weather_icon_ctx_t *ctx = (weather_icon_ctx_t *)user_ctx;
    if (ctx == NULL || ctx->target == NULL || rgb565 == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t *copy = (uint8_t *)malloc(rgb565_len);
    if (copy == NULL) {
        return ESP_ERR_NO_MEM;
    }
    memcpy(copy, rgb565, rgb565_len);
#if LV_COLOR_16_SWAP
    rgb565_swap(copy, rgb565_len);
#endif
    if (ctx->data != NULL) {
        free(ctx->data);
    }
    ctx->data = copy;
    memset(&ctx->dsc, 0, sizeof(ctx->dsc));
    ctx->dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
    ctx->dsc.header.w = width;
    ctx->dsc.header.h = height;
    ctx->dsc.data_size = rgb565_len;
    ctx->dsc.data = ctx->data;
    lv_img_set_src(ctx->target, &ctx->dsc);
    return ESP_OK;
}

static uint16_t icon_group_fallback(uint16_t code)
{
    if (code >= 200 && code <= 232) {
        return 200;
    }
    if (code >= 300 && code <= 321) {
        return 300;
    }
    if (code >= 500 && code <= 531) {
        return 500;
    }
    if (code >= 600 && code <= 622) {
        return 600;
    }
    if (code >= 700 && code <= 781) {
        return 701;
    }
    if (code == 800) {
        return 800;
    }
    if (code >= 801 && code <= 804) {
        return 801;
    }
    return 0;
}

static FILE *open_icon_bin(const char *bin_name)
{
    const char *bin = bin_name != NULL ? bin_name : "icon_150.bin";
    char path[96];
    int written = snprintf(path, sizeof(path), "%s/%s", PULSEMON_SD_MOUNT_POINT, bin);
    if (written <= 0 || (size_t)written >= sizeof(path)) {
        return NULL;
    }
    return fopen(path, "rb");
}

static esp_err_t find_icon_offset(FILE *fp, uint16_t *io_code, uint8_t variant, uint32_t *out_offset)
{
    esp_err_t err = svg2bin_find_entry_offset_stream(fp, *io_code, variant, out_offset);
    if (err == ESP_ERR_NOT_FOUND && variant != SVG2BIN_VARIANT_NEUTRAL) {
        err = svg2bin_find_entry_offset_stream(fp, *io_code, SVG2BIN_VARIANT_NEUTRAL, out_offset);
    }
    if (err == ESP_ERR_NOT_FOUND) {
        uint16_t fallback = icon_group_fallback(*io_code);
        if (fallback != 0 && fallback != *io_code) {
            err = svg2bin_find_entry_offset_stream(fp, fallback, variant, out_offset);
            if (err == ESP_ERR_NOT_FOUND && variant != SVG2BIN_VARIANT_NEUTRAL) {
                err = svg2bin_find_entry_offset_stream(fp, fallback, SVG2BIN_VARIANT_NEUTRAL, out_offset);
            }
            if (err == ESP_OK) {
                WEATHER_ICON_LOGW("icon fallback: %u -> %u", (unsigned)*io_code, (unsigned)fallback);
                *io_code = fallback;
            }
        }
    }
    return err;
}

esp_err_t pulsemon_weather_icons_init(void)
{
    esp_err_t err = sd_storage_ensure_mounted();
    if (err != ESP_OK) {
        WEATHER_ICON_LOGW("sd unavailable for weather icons: %s", esp_err_to_name(err));
        return err;
    }
#if PULSEMON_DEBUG
    sd_storage_log_file_list(PULSEMON_SD_MOUNT_POINT);
    pulsemon_weather_icons_log_index("icon_150.bin");
    pulsemon_weather_icons_log_index("icon_50.bin");
#endif
    return ESP_OK;
}

esp_err_t pulsemon_weather_icons_log_index(const char *bin_name)
{
#if !PULSEMON_DEBUG
    (void)bin_name;
    return ESP_OK;
#else
    esp_err_t err = sd_storage_ensure_mounted();
    if (err != ESP_OK) {
        return err;
    }
    const char *bin = bin_name != NULL ? bin_name : "icon_150.bin";
    FILE *fp = open_icon_bin(bin);
    if (fp == NULL) {
        WEATHER_ICON_LOGW("weather icon bin not found: %s/%s", PULSEMON_SD_MOUNT_POINT, bin);
        return ESP_ERR_NOT_FOUND;
    }

    WEATHER_ICON_LOGI("weather icon index: %s", bin);
    err = svg2bin_log_index_stream(fp, TAG);
    fclose(fp);
    return err;
#endif
}

esp_err_t pulsemon_weather_icons_set_object(lv_obj_t *target, const char *bin_name, uint16_t code, uint8_t variant)
{
    if (target == NULL || code == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t err = sd_storage_ensure_mounted();
    if (err != ESP_OK) {
        return err;
    }
    const char *bin = bin_name != NULL ? bin_name : "icon_150.bin";
    weather_icon_ctx_t *ctx = icon_ctx_for_target(target);
    if (ctx == NULL) {
        return ESP_ERR_NO_MEM;
    }
    if (ctx->has_last && ctx->last_code == code && ctx->last_variant == variant && strcmp(ctx->last_bin, bin) == 0) {
        return ESP_OK;
    }

    weather_icon_cache_t *cached = icon_cache_find(bin, code, variant);
    if (cached != NULL && icon_apply_cached(ctx, cached) == ESP_OK) {
        ctx->has_last = true;
        ctx->last_code = code;
        ctx->last_variant = variant;
        snprintf(ctx->last_bin, sizeof(ctx->last_bin), "%s", bin);
        return ESP_OK;
    }

    FILE *fp = open_icon_bin(bin);
    if (fp == NULL) {
        WEATHER_ICON_LOGW("weather icon bin not found: %s", bin);
        return ESP_ERR_NOT_FOUND;
    }

    uint16_t resolved_code = code;
    uint32_t offset = 0;
    err = find_icon_offset(fp, &resolved_code, variant, &offset);
    if (err == ESP_OK) {
        err = svg2bin_decode_entry_at_offset(fp, offset, icon_draw_cb, ctx);
    }
    fclose(fp);
    if (err != ESP_OK) {
        WEATHER_ICON_LOGW("icon decode failed code=%u variant=%u: %s", (unsigned)code, (unsigned)variant, esp_err_to_name(err));
        return err;
    }

    weather_icon_cache_t *slot = icon_cache_reserve(bin, code, variant);
    if (slot != NULL && ctx->data != NULL) {
        uint8_t *copy = (uint8_t *)malloc(ctx->dsc.data_size);
        if (copy != NULL) {
            memcpy(copy, ctx->data, ctx->dsc.data_size);
            slot->data = copy;
            slot->data_size = ctx->dsc.data_size;
            slot->width = ctx->dsc.header.w;
            slot->height = ctx->dsc.header.h;
        }
    }

    ctx->has_last = true;
    ctx->last_code = code;
    ctx->last_variant = variant;
    snprintf(ctx->last_bin, sizeof(ctx->last_bin), "%s", bin);
    return ESP_OK;
}

esp_err_t pulsemon_weather_icons_set_main(uint16_t code, uint8_t variant)
{
    return pulsemon_weather_icons_set_object(ui_weather_image(), "icon_150.bin", code, variant);
}
