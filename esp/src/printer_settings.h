#ifndef PRINTER_SETTINGS_H
#define PRINTER_SETTINGS_H

#include <stdbool.h>

#include "esp_err.h"

#define PRINTER_HOST_MAX_LEN 95
#define PRINTER_ACCESS_CODE_MAX_LEN 64

typedef struct {
    char host[PRINTER_HOST_MAX_LEN + 1];
    char access_code[PRINTER_ACCESS_CODE_MAX_LEN + 1];
} printer_settings_t;

void printer_settings_defaults(printer_settings_t *out);
bool printer_settings_validate(const printer_settings_t *settings);
esp_err_t printer_settings_load(printer_settings_t *out);
esp_err_t printer_settings_save(const printer_settings_t *settings);
esp_err_t printer_settings_clear(void);

#endif
