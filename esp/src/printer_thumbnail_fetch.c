#include "printer_thumbnail_fetch.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "mbedtls/base64.h"

#define PRINTER_THUMBNAIL_B64_MAX (192U * 1024U)
#define PRINTER_THUMBNAIL_MAX_DIM_PX 512U

static const char *TAG = "printer_thumb_fetch";

static uint32_t read_be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) |
           ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) |
           (uint32_t)p[3];
}

static bool png_dimensions(const uint8_t *data, size_t len, uint16_t *width, uint16_t *height)
{
    static const uint8_t signature[8] = {0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a, 0x0a};
    if (data == NULL || len < 24 || memcmp(data, signature, sizeof(signature)) != 0) {
        return false;
    }

    uint32_t w = read_be32(data + 16);
    uint32_t h = read_be32(data + 20);
    if (w == 0 || h == 0 || w > PRINTER_THUMBNAIL_MAX_DIM_PX || h > PRINTER_THUMBNAIL_MAX_DIM_PX) {
        return false;
    }

    if (width != NULL) {
        *width = (uint16_t)w;
    }
    if (height != NULL) {
        *height = (uint16_t)h;
    }
    return true;
}

static const char *base64_payload(const char *thumbnail)
{
    static const char data_prefix[] = "data:image/";
    if (thumbnail == NULL) {
        return NULL;
    }
    if (strncmp(thumbnail, data_prefix, sizeof(data_prefix) - 1U) != 0) {
        return thumbnail;
    }

    const char *marker = strstr(thumbnail, ";base64,");
    return marker != NULL ? marker + strlen(";base64,") : NULL;
}

printer_thumbnail_fetch_result_t printer_thumbnail_decode_base64_png(const char *thumbnail,
                                                                      uint8_t **png_data,
                                                                      size_t *png_len,
                                                                      uint16_t *png_width,
                                                                      uint16_t *png_height)
{
    if (png_data == NULL || png_len == NULL || png_width == NULL || png_height == NULL) {
        return THUMBNAIL_FETCH_NO_IMAGE;
    }

    *png_data = NULL;
    *png_len = 0;
    *png_width = 0;
    *png_height = 0;

    const char *b64 = base64_payload(thumbnail);
    if (b64 == NULL || b64[0] == '\0') {
        ESP_LOGW(TAG, "thumbnail method=1045 payload is empty or unsupported data URL");
        return THUMBNAIL_FETCH_NO_IMAGE;
    }

    size_t b64_len = strlen(b64);
    if (b64_len == 0 || b64_len > PRINTER_THUMBNAIL_B64_MAX) {
        ESP_LOGW(TAG, "thumbnail method=1045 base64 length rejected bytes=%u", (unsigned)b64_len);
        return THUMBNAIL_FETCH_NO_IMAGE;
    }

    size_t decoded_cap = (b64_len / 4U) * 3U + 3U;
    uint8_t *decoded = malloc(decoded_cap);
    if (decoded == NULL) {
        ESP_LOGW(TAG, "thumbnail method=1045 decode allocation failed bytes=%u", (unsigned)decoded_cap);
        return THUMBNAIL_FETCH_RETRY;
    }

    size_t decoded_len = 0;
    int rc = mbedtls_base64_decode(decoded,
                                   decoded_cap,
                                   &decoded_len,
                                   (const unsigned char *)b64,
                                   b64_len);
    if (rc != 0) {
        ESP_LOGW(TAG,
                 "thumbnail method=1045 base64 decode failed rc=%d input_bytes=%u",
                 rc,
                 (unsigned)b64_len);
        free(decoded);
        return THUMBNAIL_FETCH_NO_IMAGE;
    }

    uint16_t width = 0;
    uint16_t height = 0;
    if (!png_dimensions(decoded, decoded_len, &width, &height)) {
        ESP_LOGW(TAG,
                 "thumbnail method=1045 decoded payload is not a supported PNG bytes=%u",
                 (unsigned)decoded_len);
        free(decoded);
        return THUMBNAIL_FETCH_NO_IMAGE;
    }

    ESP_LOGI(TAG,
             "thumbnail method=1045 decoded PNG size=%ux%u bytes=%u base64_bytes=%u",
             (unsigned)width,
             (unsigned)height,
             (unsigned)decoded_len,
             (unsigned)b64_len);

    *png_data = decoded;
    *png_len = decoded_len;
    *png_width = width;
    *png_height = height;
    return THUMBNAIL_FETCH_OK;
}
