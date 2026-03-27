#ifndef PULSEMON_VARS_H
#define PULSEMON_VARS_H

#include <stdbool.h>

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

#endif
