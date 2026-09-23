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

const char *get_var_name_printer(void);
void set_var_name_printer(const char *value);
const char *get_var_printer_ip(void);
void set_var_printer_ip(const char *value);
const char *get_var_print_file_name(void);
void set_var_print_file_name(const char *value);
const char *get_var_print_time_start(void);
void set_var_print_time_start(const char *value);
const char *get_var_print_time_end(void);
void set_var_print_time_end(const char *value);
const char *get_var_print_time_elapsed(void);
void set_var_print_time_elapsed(const char *value);
const char *get_var_print_time_remaining(void);
void set_var_print_time_remaining(const char *value);
const char *get_var_print_layer(void);
void set_var_print_layer(const char *value);
int32_t get_var_print_bar(void);
void set_var_print_bar(int32_t value);

#endif
