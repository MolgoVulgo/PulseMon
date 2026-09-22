#ifndef PRINTER_THUMBNAIL_H
#define PRINTER_THUMBNAIL_H

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

void printer_thumbnail_init(lv_obj_t *frame);
esp_err_t printer_thumbnail_set_png(uint8_t *png_data, size_t png_len, uint16_t width, uint16_t height);
esp_err_t printer_thumbnail_clear(void);

#ifdef __cplusplus
}
#endif

#endif
