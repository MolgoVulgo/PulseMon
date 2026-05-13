#ifndef NEWS_SERVICE_H
#define NEWS_SERVICE_H

#include "esp_err.h"

esp_err_t news_service_start(void);
void news_service_request_update(void);

#endif
