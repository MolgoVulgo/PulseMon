#ifndef PULSEMON_VARS_H
#define PULSEMON_VARS_H

#include <stdbool.h>
#include <stdint.h>

#include "ui/vars.h"

typedef struct {
    float cpu_pct;
    bool cpu_pct_valid;
    float gpu_pct;
    bool gpu_pct_valid;
    float cpu_temp_c;
    bool cpu_temp_c_valid;
    float gpu_temp_c;
    bool gpu_temp_c_valid;
} vars_graph_sample_t;

void vars_get_graph_sample(vars_graph_sample_t *out);

/* Fan vars helpers (runtime-safe names) */
int32_t get_var_fan_2_rpm(void);
void set_var_fan_2_rpm(int32_t value);
int32_t vars_get_fan_1_rpm_value(void);
int32_t vars_get_fan_2_rpm_value(void);
int32_t vars_get_fan_3_rpm_value(void);

#endif
