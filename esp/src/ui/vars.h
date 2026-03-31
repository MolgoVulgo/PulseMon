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
    FLOW_GLOBAL_VARIABLE_CPU_TEMP = 1,
    FLOW_GLOBAL_VARIABLE_MEM_PCT = 2,
    FLOW_GLOBAL_VARIABLE_MEM_USED = 3,
    FLOW_GLOBAL_VARIABLE_MEM_TOTAL = 4,
    FLOW_GLOBAL_VARIABLE_GPU_PCT = 5,
    FLOW_GLOBAL_VARIABLE_GPU_TEMP = 6,
    FLOW_GLOBAL_VARIABLE_GPU_POWER = 7,
    FLOW_GLOBAL_VARIABLE_HOST_META = 8,
    FLOW_GLOBAL_VARIABLE_GPU_VRAM_TOTAL = 9,
    FLOW_GLOBAL_VARIABLE_GPU_MEM_CLOCK = 10,
    FLOW_GLOBAL_VARIABLE_GPU_FAN_RPM = 11,
    FLOW_GLOBAL_VARIABLE_GPU_VRAM_USED = 12
};

// Native global variables

extern const char *get_var_cpu_pct();
extern void set_var_cpu_pct(const char *value);
extern const char *get_var_cpu_temp();
extern void set_var_cpu_temp(const char *value);
extern const char *get_var_mem_pct();
extern void set_var_mem_pct(const char *value);
extern const char *get_var_mem_used();
extern void set_var_mem_used(const char *value);
extern const char *get_var_mem_total();
extern void set_var_mem_total(const char *value);
extern const char *get_var_gpu_pct();
extern void set_var_gpu_pct(const char *value);
extern const char *get_var_gpu_temp();
extern void set_var_gpu_temp(const char *value);
extern const char *get_var_gpu_power();
extern void set_var_gpu_power(const char *value);
extern const char *get_var_host_meta();
extern void set_var_host_meta(const char *value);
extern const char *get_var_gpu_vram_total();
extern void set_var_gpu_vram_total(const char *value);
extern const char *get_var_gpu_mem_clock();
extern void set_var_gpu_mem_clock(const char *value);
extern const char *get_var_gpu_fan_rpm();
extern void set_var_gpu_fan_rpm(const char *value);
extern int32_t get_var_gpu_vram_used();
extern void set_var_gpu_vram_used(int32_t value);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/