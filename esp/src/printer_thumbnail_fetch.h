#ifndef PRINTER_THUMBNAIL_FETCH_H
#define PRINTER_THUMBNAIL_FETCH_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    THUMBNAIL_FETCH_OK = 0,
    THUMBNAIL_FETCH_NO_IMAGE,
    THUMBNAIL_FETCH_RETRY,
    THUMBNAIL_FETCH_NOT_FOUND,
} printer_thumbnail_fetch_result_t;

/* Decode a method-1045 thumbnail. On success, caller owns *png_data. */
printer_thumbnail_fetch_result_t printer_thumbnail_decode_base64_png(const char *thumbnail,
                                                                      uint8_t **png_data,
                                                                      size_t *png_len,
                                                                      uint16_t *png_width,
                                                                      uint16_t *png_height);

#ifdef __cplusplus
}
#endif

#endif
