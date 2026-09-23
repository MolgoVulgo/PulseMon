#ifndef PRINTER_SERVICE_H
#define PRINTER_SERVICE_H

#include <stdbool.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t printer_service_init(void);
esp_err_t printer_service_start(void);
void printer_service_stop(void);
void printer_service_request_update(void);
void printer_service_reload_settings(void);
bool printer_service_is_available(void);
bool printer_service_has_cached_display(void);
bool printer_service_restore_cached_display(void);

#ifdef __cplusplus
}
#endif

#endif
