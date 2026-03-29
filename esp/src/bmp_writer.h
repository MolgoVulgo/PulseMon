#pragma once

#include <stdbool.h>
#include <stdint.h>

bool bmp_write_rgb565_file(const char *path, const uint16_t *rgb565, uint16_t width, uint16_t height);
