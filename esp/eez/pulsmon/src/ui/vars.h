#ifndef EEZ_LVGL_UI_VARS_H
#define EEZ_LVGL_UI_VARS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// enum declarations

// Flow global variables

enum FlowGlobalVariables {
    FLOW_GLOBAL_VARIABLE_CPU_PCT = 0,
    FLOW_GLOBAL_VARIABLE_CPU_TEMP = 1
};

// Native global variables

extern const char *get_var_cpu_pct();
extern void set_var_cpu_pct(const char *value);
extern const char *get_var_cpu_temp();
extern void set_var_cpu_temp(const char *value);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/