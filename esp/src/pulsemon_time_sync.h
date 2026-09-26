#ifndef PULSEMON_TIME_SYNC_H
#define PULSEMON_TIME_SYNC_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

esp_err_t pulsemon_time_sync_start(void);
bool pulsemon_time_sync_wait(uint32_t timeout_ms);
bool pulsemon_time_sync_is_valid(void);

#endif
