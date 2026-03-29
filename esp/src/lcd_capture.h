#pragma once

#include <stdint.h>
#include "esp_err.h"

typedef struct {
    uint16_t width;
    uint16_t height;
    uint32_t interval_ms;
    const char *output_dir;
} lcd_capture_cfg_t;

esp_err_t lcd_capture_start(const lcd_capture_cfg_t *cfg);
void lcd_capture_on_flush_chunk(int x, int y, int width, int height, const void *rgb565_chunk);
