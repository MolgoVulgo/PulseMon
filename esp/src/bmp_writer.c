#include "bmp_writer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "lv_conf.h"

static const char *TAG = "bmp_writer";

static inline void le16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
}

static inline void le32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
    p[2] = (uint8_t)((v >> 16) & 0xFFu);
    p[3] = (uint8_t)((v >> 24) & 0xFFu);
}

static inline void rgb565_to_bgr24(uint16_t px, uint8_t *bgr)
{
#if LV_COLOR_16_SWAP
    /* LVGL buffer is byte-swapped for the panel bus; restore canonical RGB565 for file export. */
    px = (uint16_t)((px << 8) | (px >> 8));
#endif

    uint8_t r5 = (uint8_t)((px >> 11) & 0x1Fu);
    uint8_t g6 = (uint8_t)((px >> 5) & 0x3Fu);
    uint8_t b5 = (uint8_t)(px & 0x1Fu);

    uint8_t r8 = (uint8_t)((r5 << 3) | (r5 >> 2));
    uint8_t g8 = (uint8_t)((g6 << 2) | (g6 >> 4));
    uint8_t b8 = (uint8_t)((b5 << 3) | (b5 >> 2));

    bgr[0] = b8;
    bgr[1] = g8;
    bgr[2] = r8;
}

bool bmp_write_rgb565_file(const char *path, const uint16_t *rgb565, uint16_t width, uint16_t height)
{
    if (path == NULL || rgb565 == NULL || width == 0 || height == 0) {
        return false;
    }

    FILE *f = fopen(path, "wb");
    if (f == NULL) {
        ESP_LOGE(TAG, "open failed: %s", path);
        return false;
    }

    const uint32_t row_raw = (uint32_t)width * 3u;
    const uint32_t row_stride = (row_raw + 3u) & ~3u;
    const uint32_t pixel_data_size = row_stride * (uint32_t)height;
    const uint32_t file_size = 14u + 40u + pixel_data_size;

    uint8_t file_hdr[14] = {0};
    file_hdr[0] = 'B';
    file_hdr[1] = 'M';
    le32(&file_hdr[2], file_size);
    le32(&file_hdr[10], 14u + 40u);

    uint8_t dib_hdr[40] = {0};
    le32(&dib_hdr[0], 40u);
    le32(&dib_hdr[4], width);
    le32(&dib_hdr[8], height);
    le16(&dib_hdr[12], 1u);
    le16(&dib_hdr[14], 24u);
    le32(&dib_hdr[20], pixel_data_size);

    if (fwrite(file_hdr, 1, sizeof(file_hdr), f) != sizeof(file_hdr) ||
        fwrite(dib_hdr, 1, sizeof(dib_hdr), f) != sizeof(dib_hdr)) {
        ESP_LOGE(TAG, "header write failed: %s", path);
        fclose(f);
        return false;
    }

    uint8_t *row_buf = (uint8_t *)malloc(row_stride);
    if (row_buf == NULL) {
        ESP_LOGE(TAG, "row buffer alloc failed");
        fclose(f);
        return false;
    }

    for (int y = (int)height - 1; y >= 0; --y) {
        const uint16_t *src = rgb565 + ((size_t)y * width);
        for (uint16_t x = 0; x < width; ++x) {
            rgb565_to_bgr24(src[x], &row_buf[(size_t)x * 3u]);
        }

        if (row_stride > row_raw) {
            memset(row_buf + row_raw, 0, row_stride - row_raw);
        }

        if (fwrite(row_buf, 1, row_stride, f) != row_stride) {
            ESP_LOGE(TAG, "pixel write failed at row=%d: %s", y, path);
            free(row_buf);
            fclose(f);
            return false;
        }
    }

    free(row_buf);
    if (fclose(f) != 0) {
        ESP_LOGE(TAG, "close failed: %s", path);
        return false;
    }

    return true;
}
