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

const char *get_var_gpu_fan_rpm_1(void);
void set_var_gpu_fan_rpm_1(const char *value);

/* Fan numeric helpers for runtime patches. Generated RPM vars are text labels. */
int32_t vars_get_fan_1_rpm_value(void);
int32_t vars_get_fan_2_rpm_value(void);
int32_t vars_get_fan_3_rpm_value(void);

#endif
