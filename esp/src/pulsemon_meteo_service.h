#ifndef PULSEMON_METEO_SERVICE_H
#define PULSEMON_METEO_SERVICE_H

#include "esp_err.h"

esp_err_t pulsemon_meteo_service_start(void);
void pulsemon_meteo_service_request_update(void);

#endif
