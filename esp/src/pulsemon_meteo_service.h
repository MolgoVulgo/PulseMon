#ifndef PULSEMON_METEO_SERVICE_H
#define PULSEMON_METEO_SERVICE_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

esp_err_t pulsemon_meteo_service_start(void);
void pulsemon_meteo_service_request_update(void);
bool pulsemon_meteo_service_request_update_and_wait(uint32_t timeout_ms);

#endif
