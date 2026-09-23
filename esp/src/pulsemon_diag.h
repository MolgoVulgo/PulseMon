#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t pulsemon_diag_init(void);
void pulsemon_diag_heap(const char *owner, const char *stage);
void pulsemon_diag_stack(const char *owner, const char *stage);

#ifdef __cplusplus
}
#endif
