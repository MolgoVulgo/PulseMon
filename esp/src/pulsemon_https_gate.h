#pragma once

#include <stdbool.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t pulsemon_https_gate_init(void);
bool pulsemon_https_gate_acquire(const char *owner, TickType_t wait_ticks);
void pulsemon_https_gate_release(const char *owner);

#ifdef __cplusplus
}
#endif
