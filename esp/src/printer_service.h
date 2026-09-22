#ifndef PRINTER_SERVICE_H
#define PRINTER_SERVICE_H

#include <stdbool.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t printer_service_start(void);
void printer_service_request_update(void);
bool printer_service_is_available(void);

#ifdef __cplusplus
}
#endif

#endif
