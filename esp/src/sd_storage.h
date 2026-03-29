#pragma once

#include <stdbool.h>
#include "esp_err.h"

esp_err_t sd_storage_ensure_mounted(void);
bool sd_storage_is_mounted(void);
void sd_storage_deinit(void);
