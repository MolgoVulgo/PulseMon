#ifndef NEWS_SERVICE_H
#define NEWS_SERVICE_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

esp_err_t news_service_start(void);
void news_service_request_update(void);
bool news_service_request_update_and_wait(uint32_t timeout_ms);

#endif
