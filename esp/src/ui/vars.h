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
    FLOW_GLOBAL_VARIABLE_GPU_VRAM_USED = 12,
    FLOW_GLOBAL_VARIABLE_FAN_1_LABEL = 13,
    FLOW_GLOBAL_VARIABLE_FAN_1_RPM = 14,
    FLOW_GLOBAL_VARIABLE_FAN_2_LABEL = 15,
    FLOW_GLOBAL_VARIABLE_FAN_2_RPM = 16,
    FLOW_GLOBAL_VARIABLE_FAN_3_RPM = 17,
    FLOW_GLOBAL_VARIABLE_FAN_3_LABEL = 18,
    FLOW_GLOBAL_VARIABLE_FAN_1_PCT = 19,
    FLOW_GLOBAL_VARIABLE_FAN_2_PCT = 20,
    FLOW_GLOBAL_VARIABLE_FAN_3_PCT = 21,
    FLOW_GLOBAL_VARIABLE_UI_METEO_CONDITION = 22,
    FLOW_GLOBAL_VARIABLE_UI_METEO_DATE = 23,
    FLOW_GLOBAL_VARIABLE_UI_METEO_FD1 = 24,
    FLOW_GLOBAL_VARIABLE_UI_METEO_FD2 = 25,
    FLOW_GLOBAL_VARIABLE_UI_METEO_FD3 = 26,
    FLOW_GLOBAL_VARIABLE_UI_METEO_FD4 = 27,
    FLOW_GLOBAL_VARIABLE_UI_METEO_FD5 = 28,
    FLOW_GLOBAL_VARIABLE_UI_METEO_FD6 = 29,
    FLOW_GLOBAL_VARIABLE_UI_METEO_FT1 = 30,
    FLOW_GLOBAL_VARIABLE_UI_METEO_FT2 = 31,
    FLOW_GLOBAL_VARIABLE_UI_METEO_FT3 = 32,
    FLOW_GLOBAL_VARIABLE_UI_METEO_FT4 = 33,
    FLOW_GLOBAL_VARIABLE_UI_METEO_FT5 = 34,
    FLOW_GLOBAL_VARIABLE_UI_METEO_FT6 = 35,
    FLOW_GLOBAL_VARIABLE_UI_METEO_HOURE = 36,
    FLOW_GLOBAL_VARIABLE_UI_METEO_TEMP = 37,
    FLOW_GLOBAL_VARIABLE_UI_METEO_ALERT = 38,
    FLOW_GLOBAL_VARIABLE_UI_START_BAR = 39,
    FLOW_GLOBAL_VARIABLE_UI_START_BAR_TEXTE = 40,
    FLOW_GLOBAL_VARIABLE_GPU_FAN_RPM_1 = 41
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
extern const char *get_var_fan_1_label();
extern void set_var_fan_1_label(const char *value);
extern const char *get_var_fan_1_rpm();
extern void set_var_fan_1_rpm(const char *value);
extern const char *get_var_fan_2_label();
extern void set_var_fan_2_label(const char *value);
extern const char *get_var_fan_2_rpm();
extern void set_var_fan_2_rpm(const char *value);
extern const char *get_var_fan_3_rpm();
extern void set_var_fan_3_rpm(const char *value);
extern const char *get_var_fan_3_label();
extern void set_var_fan_3_label(const char *value);
extern int32_t get_var_fan_1_pct();
extern void set_var_fan_1_pct(int32_t value);
extern int32_t get_var_fan_2_pct();
extern void set_var_fan_2_pct(int32_t value);
extern int32_t get_var_fan_3_pct();
extern void set_var_fan_3_pct(int32_t value);
extern const char *get_var_ui_meteo_condition();
extern void set_var_ui_meteo_condition(const char *value);
extern const char *get_var_ui_meteo_date();
extern void set_var_ui_meteo_date(const char *value);
extern const char *get_var_ui_meteo_fd1();
extern void set_var_ui_meteo_fd1(const char *value);
extern const char *get_var_ui_meteo_fd2();
extern void set_var_ui_meteo_fd2(const char *value);
extern const char *get_var_ui_meteo_fd3();
extern void set_var_ui_meteo_fd3(const char *value);
extern const char *get_var_ui_meteo_fd4();
extern void set_var_ui_meteo_fd4(const char *value);
extern const char *get_var_ui_meteo_fd5();
extern void set_var_ui_meteo_fd5(const char *value);
extern const char *get_var_ui_meteo_fd6();
extern void set_var_ui_meteo_fd6(const char *value);
extern const char *get_var_ui_meteo_ft1();
extern void set_var_ui_meteo_ft1(const char *value);
extern const char *get_var_ui_meteo_ft2();
extern void set_var_ui_meteo_ft2(const char *value);
extern const char *get_var_ui_meteo_ft3();
extern void set_var_ui_meteo_ft3(const char *value);
extern const char *get_var_ui_meteo_ft4();
extern void set_var_ui_meteo_ft4(const char *value);
extern const char *get_var_ui_meteo_ft5();
extern void set_var_ui_meteo_ft5(const char *value);
extern const char *get_var_ui_meteo_ft6();
extern void set_var_ui_meteo_ft6(const char *value);
extern const char *get_var_ui_meteo_houre();
extern void set_var_ui_meteo_houre(const char *value);
extern const char *get_var_ui_meteo_temp();
extern void set_var_ui_meteo_temp(const char *value);
extern const char *get_var_ui_meteo_alert();
extern void set_var_ui_meteo_alert(const char *value);
extern int32_t get_var_ui_start_bar();
extern void set_var_ui_start_bar(int32_t value);
extern const char *get_var_ui_start_bar_texte();
extern void set_var_ui_start_bar_texte(const char *value);
extern const char *get_var_gpu_fan_rpm_1();
extern void set_var_gpu_fan_rpm_1(const char *value);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/