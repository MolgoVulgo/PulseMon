#include "lcd_capture.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "bmp_writer.h"
#include "capture_config.h"
#include "sd_storage.h"

static const char *TAG = "lcd_capture";

#if PULSEMON_SCREENSHOT_DEBUG

typedef struct {
    uint16_t width;
    uint16_t height;
    uint32_t interval_ms;
    char output_dir[128];

    size_t frame_bytes;
    uint16_t *live_fb;
    uint16_t *snap_fb;

    SemaphoreHandle_t fb_mutex;
    TaskHandle_t task;

    bool started;
    bool has_frame;
    uint32_t index;
} lcd_capture_state_t;

static lcd_capture_state_t s;

static void reset_state(void)
{
    if (s.task != NULL) {
        vTaskDelete(s.task);
        s.task = NULL;
    }
    if (s.fb_mutex != NULL) {
        vSemaphoreDelete(s.fb_mutex);
        s.fb_mutex = NULL;
    }
    if (s.live_fb != NULL) {
        heap_caps_free(s.live_fb);
        s.live_fb = NULL;
    }
    if (s.snap_fb != NULL) {
        heap_caps_free(s.snap_fb);
        s.snap_fb = NULL;
    }
    memset(&s, 0, sizeof(s));
}

static bool mkdir_if_needed(const char *path)
{
    struct stat st;
    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode);
    }

    if (mkdir(path, 0755) == 0) {
        return true;
    }

    if (errno == EEXIST) {
        return true;
    }

    ESP_LOGE(TAG, "mkdir failed path=%s errno=%d", path, errno);
    return false;
}

static bool ensure_output_tree(const char *dir)
{
    if (!mkdir_if_needed(PULSEMON_SD_MOUNT_POINT)) {
        return false;
    }

    char buf[128];
    size_t len = strnlen(dir, sizeof(buf) - 1);
    if (len == 0 || len >= sizeof(buf)) {
        ESP_LOGE(TAG, "invalid output dir");
        return false;
    }

    memcpy(buf, dir, len);
    buf[len] = '\0';

    for (size_t i = 1; i < len; ++i) {
        if (buf[i] == '/') {
            buf[i] = '\0';
            if (!mkdir_if_needed(buf)) {
                return false;
            }
            buf[i] = '/';
        }
    }

    return mkdir_if_needed(buf);
}

static bool take_snapshot(void)
{
    if (s.fb_mutex == NULL || s.live_fb == NULL || s.snap_fb == NULL) {
        return false;
    }

    if (xSemaphoreTake(s.fb_mutex, pdMS_TO_TICKS(20)) != pdTRUE) {
        ESP_LOGW(TAG, "snapshot skipped: framebuffer busy");
        return false;
    }

    bool has_frame = s.has_frame;
    if (has_frame) {
        memcpy(s.snap_fb, s.live_fb, s.frame_bytes);
    }
    xSemaphoreGive(s.fb_mutex);

    if (!has_frame) {
        ESP_LOGW(TAG, "snapshot skipped: framebuffer not ready");
        return false;
    }

    return true;
}

static bool next_output_path(char *out, size_t out_sz)
{
    for (uint32_t tries = 0; tries < 1000000; ++tries) {
        uint32_t n = ++s.index;
        int ret = snprintf(out, out_sz, "%s/cap_%06lu.bmp", s.output_dir, (unsigned long)n);
        if (ret <= 0 || (size_t)ret >= out_sz) {
            ESP_LOGE(TAG, "output path too long");
            return false;
        }

        if (access(out, F_OK) != 0) {
            return true;
        }
    }

    ESP_LOGE(TAG, "unable to allocate file name");
    return false;
}

static void capture_task(void *arg)
{
    (void)arg;

    TickType_t last = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(s.interval_ms);

    ESP_LOGI(TAG, "capture task started interval=%lums dir=%s", (unsigned long)s.interval_ms, s.output_dir);

    while (1) {
        vTaskDelayUntil(&last, period);

        int64_t t0 = esp_timer_get_time();
        esp_err_t mret = sd_storage_ensure_mounted();
        if (mret != ESP_OK) {
            ESP_LOGE(TAG, "capture failed: sd not mounted (%s)", esp_err_to_name(mret));
            continue;
        }

        if (!ensure_output_tree(s.output_dir)) {
            ESP_LOGE(TAG, "capture failed: output dir unavailable (%s)", s.output_dir);
            continue;
        }

        if (!take_snapshot()) {
            continue;
        }

        char path[192];
        if (!next_output_path(path, sizeof(path))) {
            ESP_LOGE(TAG, "capture failed: path generation");
            continue;
        }

        if (!bmp_write_rgb565_file(path, s.snap_fb, s.width, s.height)) {
            ESP_LOGE(TAG, "capture failed: bmp write (%s)", path);
            continue;
        }

        int64_t elapsed_ms = (esp_timer_get_time() - t0) / 1000;
        ESP_LOGI(TAG, "SCREENSHOT TAKEN file=%s elapsed=%lldms", path, (long long)elapsed_ms);
    }
}

esp_err_t lcd_capture_start(const lcd_capture_cfg_t *cfg)
{
    if (cfg == NULL || cfg->width == 0 || cfg->height == 0 || cfg->interval_ms == 0 || cfg->output_dir == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (s.started) {
        return ESP_OK;
    }

    memset(&s, 0, sizeof(s));
    s.width = cfg->width;
    s.height = cfg->height;
    s.interval_ms = cfg->interval_ms;
    s.frame_bytes = (size_t)s.width * (size_t)s.height * sizeof(uint16_t);
    snprintf(s.output_dir, sizeof(s.output_dir), "%s", cfg->output_dir);

    s.live_fb = (uint16_t *)heap_caps_malloc(s.frame_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    s.snap_fb = (uint16_t *)heap_caps_malloc(s.frame_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (s.live_fb == NULL || s.snap_fb == NULL) {
        ESP_LOGE(TAG, "framebuffer alloc failed bytes=%u", (unsigned)s.frame_bytes);
        reset_state();
        return ESP_ERR_NO_MEM;
    }

    memset(s.live_fb, 0, s.frame_bytes);
    memset(s.snap_fb, 0, s.frame_bytes);

    s.fb_mutex = xSemaphoreCreateMutex();
    if (s.fb_mutex == NULL) {
        ESP_LOGE(TAG, "mutex alloc failed");
        reset_state();
        return ESP_ERR_NO_MEM;
    }

    BaseType_t ok = xTaskCreate(capture_task, "lcd_capture", 8192, NULL, 3, &s.task);
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "capture task create failed");
        reset_state();
        return ESP_FAIL;
    }

    s.started = true;
    ESP_LOGI(TAG, "capture enabled (%ux%u, every %lums)",
             (unsigned)s.width,
             (unsigned)s.height,
             (unsigned long)s.interval_ms);
    return ESP_OK;
}

void lcd_capture_on_flush_chunk(int x, int y, int width, int height, const void *rgb565_chunk)
{
    if (!s.started || rgb565_chunk == NULL || width <= 0 || height <= 0) {
        return;
    }

    if (x < 0 || y < 0) {
        return;
    }

    if ((uint32_t)x >= s.width || (uint32_t)y >= s.height) {
        return;
    }

    uint32_t w = (uint32_t)width;
    uint32_t h = (uint32_t)height;

    if ((uint32_t)x + w > s.width) {
        w = s.width - (uint32_t)x;
    }
    if ((uint32_t)y + h > s.height) {
        h = s.height - (uint32_t)y;
    }

    if (w == 0 || h == 0) {
        return;
    }

    if (xSemaphoreTake(s.fb_mutex, 0) != pdTRUE) {
        return;
    }

    const uint16_t *src = (const uint16_t *)rgb565_chunk;
    for (uint32_t row = 0; row < h; ++row) {
        uint16_t *dst_row = s.live_fb + ((size_t)(y + (int)row) * s.width) + (uint32_t)x;
        const uint16_t *src_row = src + ((size_t)row * (uint32_t)width);
        memcpy(dst_row, src_row, (size_t)w * sizeof(uint16_t));
    }

    s.has_frame = true;
    xSemaphoreGive(s.fb_mutex);
}

#else

esp_err_t lcd_capture_start(const lcd_capture_cfg_t *cfg)
{
    (void)cfg;
    ESP_LOGI(TAG, "capture disabled (PULSEMON_SCREENSHOT_DEBUG=0)");
    return ESP_OK;
}

void lcd_capture_on_flush_chunk(int x, int y, int width, int height, const void *rgb565_chunk)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)rgb565_chunk;
}

#endif
