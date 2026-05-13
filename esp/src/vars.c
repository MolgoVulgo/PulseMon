#include <ctype.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vars.h"

#define VAR_BUF_LEN 64
#define HOST_META_BUF_LEN 160
#define METEO_ALERT_BUF_LEN 960

static char g_cpu_pct[VAR_BUF_LEN];
static char g_cpu_temp[VAR_BUF_LEN];
static char g_mem_pct[VAR_BUF_LEN];
static char g_mem_used[VAR_BUF_LEN];
static char g_mem_total[VAR_BUF_LEN];
static char g_gpu_pct[VAR_BUF_LEN];
static char g_gpu_temp[VAR_BUF_LEN];
static char g_gpu_power[VAR_BUF_LEN];
static char g_gpu_vram_total[VAR_BUF_LEN];
static char g_gpu_mem_clock[VAR_BUF_LEN];
static char g_gpu_fan_rpm[VAR_BUF_LEN];
static char g_host_meta[HOST_META_BUF_LEN];
static int32_t g_gpu_vram_used;
static char g_ui_meteo_condition[VAR_BUF_LEN];
static char g_ui_meteo_date[VAR_BUF_LEN];
static char g_ui_meteo_fd1[VAR_BUF_LEN];
static char g_ui_meteo_fd2[VAR_BUF_LEN];
static char g_ui_meteo_fd3[VAR_BUF_LEN];
static char g_ui_meteo_fd4[VAR_BUF_LEN];
static char g_ui_meteo_fd5[VAR_BUF_LEN];
static char g_ui_meteo_fd6[VAR_BUF_LEN];
static char g_ui_meteo_ft1[VAR_BUF_LEN];
static char g_ui_meteo_ft2[VAR_BUF_LEN];
static char g_ui_meteo_ft3[VAR_BUF_LEN];
static char g_ui_meteo_ft4[VAR_BUF_LEN];
static char g_ui_meteo_ft5[VAR_BUF_LEN];
static char g_ui_meteo_ft6[VAR_BUF_LEN];
static char g_ui_meteo_houre[VAR_BUF_LEN];
static char g_ui_meteo_temp[VAR_BUF_LEN];
static char g_ui_meteo_alert[METEO_ALERT_BUF_LEN];
static int32_t g_ui_start_bar;
static char g_ui_start_bar_texte[VAR_BUF_LEN];
static char g_fan_1_label[VAR_BUF_LEN];
static int32_t g_fan_1_rpm;
static char g_fan_1_rpm_text[VAR_BUF_LEN];
static char g_fan_2_label[VAR_BUF_LEN];
static int32_t g_fan_2_rpm;
static char g_fan_2_rpm_text[VAR_BUF_LEN];
static char g_fan_3_label[VAR_BUF_LEN];
static int32_t g_fan_3_rpm;
static char g_fan_3_rpm_text[VAR_BUF_LEN];
static int32_t g_fan_1_pct;
static int32_t g_fan_2_pct;
static int32_t g_fan_3_pct;

typedef struct {
    float value;
    bool valid;
} numeric_var_t;

static numeric_var_t g_cpu_pct_num;
static numeric_var_t g_gpu_pct_num;
static numeric_var_t g_cpu_temp_num;
static numeric_var_t g_gpu_temp_num;

static void set_text(char *dst, size_t cap, const char *value)
{
    if (cap == 0) {
        return;
    }
    if (value == NULL) {
        dst[0] = '\0';
        return;
    }
    strncpy(dst, value, cap - 1);
    dst[cap - 1] = '\0';
}

static bool parse_float(const char *value, float *out)
{
    if (value == NULL || out == NULL) {
        return false;
    }

    while (*value != '\0' && isspace((unsigned char)*value)) {
        value++;
    }
    if (*value == '\0') {
        return false;
    }

    errno = 0;
    char *end = NULL;
    float parsed = strtof(value, &end);
    if (value == end) {
        return false;
    }
    if (errno == ERANGE) {
        return false;
    }

    *out = parsed;
    return true;
}

static void set_numeric_from_text(numeric_var_t *target, const char *text)
{
    float parsed = 0.0f;
    target->valid = parse_float(text, &parsed);
    if (target->valid) {
        target->value = parsed;
    }
}

static int32_t parse_non_negative_int_or_zero(const char *value)
{
    if (value == NULL) {
        return 0;
    }

    while (*value != '\0' && isspace((unsigned char)*value)) {
        value++;
    }
    if (*value == '\0') {
        return 0;
    }

    errno = 0;
    char *end = NULL;
    long parsed = strtol(value, &end, 10);
    if (value == end || errno == ERANGE || parsed < 0) {
        return 0;
    }
    if (parsed > INT32_MAX) {
        return INT32_MAX;
    }
    return (int32_t)parsed;
}

const char *get_var_cpu_pct()
{
    return g_cpu_pct;
}

void set_var_cpu_pct(const char *value)
{
    set_text(g_cpu_pct, sizeof(g_cpu_pct), value);
    set_numeric_from_text(&g_cpu_pct_num, g_cpu_pct);
}

const char *get_var_cpu_temp()
{
    return g_cpu_temp;
}

void set_var_cpu_temp(const char *value)
{
    set_text(g_cpu_temp, sizeof(g_cpu_temp), value);
    set_numeric_from_text(&g_cpu_temp_num, g_cpu_temp);
}

const char *get_var_mem_pct()
{
    return g_mem_pct;
}

void set_var_mem_pct(const char *value)
{
    set_text(g_mem_pct, sizeof(g_mem_pct), value);
}

const char *get_var_mem_used()
{
    return g_mem_used;
}

void set_var_mem_used(const char *value)
{
    set_text(g_mem_used, sizeof(g_mem_used), value);
}

const char *get_var_mem_total()
{
    return g_mem_total;
}

void set_var_mem_total(const char *value)
{
    set_text(g_mem_total, sizeof(g_mem_total), value);
}

const char *get_var_gpu_pct()
{
    return g_gpu_pct;
}

void set_var_gpu_pct(const char *value)
{
    set_text(g_gpu_pct, sizeof(g_gpu_pct), value);
    set_numeric_from_text(&g_gpu_pct_num, g_gpu_pct);
}

const char *get_var_gpu_temp()
{
    return g_gpu_temp;
}

void set_var_gpu_temp(const char *value)
{
    set_text(g_gpu_temp, sizeof(g_gpu_temp), value);
    set_numeric_from_text(&g_gpu_temp_num, g_gpu_temp);
}

const char *get_var_gpu_power()
{
    return g_gpu_power;
}

void set_var_gpu_power(const char *value)
{
    set_text(g_gpu_power, sizeof(g_gpu_power), value);
}

const char *get_var_gpu_vram_total()
{
    return g_gpu_vram_total;
}

void set_var_gpu_vram_total(const char *value)
{
    set_text(g_gpu_vram_total, sizeof(g_gpu_vram_total), value);
}

const char *get_var_gpu_mem_clock()
{
    return g_gpu_mem_clock;
}

void set_var_gpu_mem_clock(const char *value)
{
    set_text(g_gpu_mem_clock, sizeof(g_gpu_mem_clock), value);
}

const char *get_var_gpu_fan_rpm()
{
    return g_gpu_fan_rpm;
}

void set_var_gpu_fan_rpm(const char *value)
{
    set_text(g_gpu_fan_rpm, sizeof(g_gpu_fan_rpm), value);
}

int32_t get_var_gpu_vram_used()
{
    return g_gpu_vram_used;
}

void set_var_gpu_vram_used(int32_t value)
{
    if (value < 0) {
        g_gpu_vram_used = 0;
        return;
    }
    if (value > 100) {
        g_gpu_vram_used = 100;
        return;
    }
    g_gpu_vram_used = value;
}

const char *get_var_host_meta()
{
    return g_host_meta;
}

void set_var_host_meta(const char *value)
{
    set_text(g_host_meta, sizeof(g_host_meta), value);
}

const char *get_var_ui_meteo_condition()
{
    return g_ui_meteo_condition;
}

void set_var_ui_meteo_condition(const char *value)
{
    set_text(g_ui_meteo_condition, sizeof(g_ui_meteo_condition), value);
}

const char *get_var_ui_meteo_date()
{
    return g_ui_meteo_date;
}

void set_var_ui_meteo_date(const char *value)
{
    set_text(g_ui_meteo_date, sizeof(g_ui_meteo_date), value);
}

const char *get_var_ui_meteo_fd1()
{
    return g_ui_meteo_fd1;
}

void set_var_ui_meteo_fd1(const char *value)
{
    set_text(g_ui_meteo_fd1, sizeof(g_ui_meteo_fd1), value);
}

const char *get_var_ui_meteo_fd2()
{
    return g_ui_meteo_fd2;
}

void set_var_ui_meteo_fd2(const char *value)
{
    set_text(g_ui_meteo_fd2, sizeof(g_ui_meteo_fd2), value);
}

const char *get_var_ui_meteo_fd3()
{
    return g_ui_meteo_fd3;
}

void set_var_ui_meteo_fd3(const char *value)
{
    set_text(g_ui_meteo_fd3, sizeof(g_ui_meteo_fd3), value);
}

const char *get_var_ui_meteo_fd4()
{
    return g_ui_meteo_fd4;
}

void set_var_ui_meteo_fd4(const char *value)
{
    set_text(g_ui_meteo_fd4, sizeof(g_ui_meteo_fd4), value);
}

const char *get_var_ui_meteo_fd5()
{
    return g_ui_meteo_fd5;
}

void set_var_ui_meteo_fd5(const char *value)
{
    set_text(g_ui_meteo_fd5, sizeof(g_ui_meteo_fd5), value);
}

const char *get_var_ui_meteo_fd6()
{
    return g_ui_meteo_fd6;
}

void set_var_ui_meteo_fd6(const char *value)
{
    set_text(g_ui_meteo_fd6, sizeof(g_ui_meteo_fd6), value);
}

const char *get_var_ui_meteo_ft1()
{
    return g_ui_meteo_ft1;
}

void set_var_ui_meteo_ft1(const char *value)
{
    set_text(g_ui_meteo_ft1, sizeof(g_ui_meteo_ft1), value);
}

const char *get_var_ui_meteo_ft2()
{
    return g_ui_meteo_ft2;
}

void set_var_ui_meteo_ft2(const char *value)
{
    set_text(g_ui_meteo_ft2, sizeof(g_ui_meteo_ft2), value);
}

const char *get_var_ui_meteo_ft3()
{
    return g_ui_meteo_ft3;
}

void set_var_ui_meteo_ft3(const char *value)
{
    set_text(g_ui_meteo_ft3, sizeof(g_ui_meteo_ft3), value);
}

const char *get_var_ui_meteo_ft4()
{
    return g_ui_meteo_ft4;
}

void set_var_ui_meteo_ft4(const char *value)
{
    set_text(g_ui_meteo_ft4, sizeof(g_ui_meteo_ft4), value);
}

const char *get_var_ui_meteo_ft5()
{
    return g_ui_meteo_ft5;
}

void set_var_ui_meteo_ft5(const char *value)
{
    set_text(g_ui_meteo_ft5, sizeof(g_ui_meteo_ft5), value);
}

const char *get_var_ui_meteo_ft6()
{
    return g_ui_meteo_ft6;
}

void set_var_ui_meteo_ft6(const char *value)
{
    set_text(g_ui_meteo_ft6, sizeof(g_ui_meteo_ft6), value);
}

const char *get_var_ui_meteo_houre()
{
    return g_ui_meteo_houre;
}

void set_var_ui_meteo_houre(const char *value)
{
    set_text(g_ui_meteo_houre, sizeof(g_ui_meteo_houre), value);
}

const char *get_var_ui_meteo_temp()
{
    return g_ui_meteo_temp;
}

void set_var_ui_meteo_temp(const char *value)
{
    set_text(g_ui_meteo_temp, sizeof(g_ui_meteo_temp), value);
}

const char *get_var_ui_meteo_alert()
{
    return g_ui_meteo_alert;
}

void set_var_ui_meteo_alert(const char *value)
{
    set_text(g_ui_meteo_alert, sizeof(g_ui_meteo_alert), value);
}

int32_t get_var_ui_start_bar()
{
    return g_ui_start_bar;
}

void set_var_ui_start_bar(int32_t value)
{
    g_ui_start_bar = value;
}

const char *get_var_ui_start_bar_texte()
{
    return g_ui_start_bar_texte;
}

void set_var_ui_start_bar_texte(const char *value)
{
    set_text(g_ui_start_bar_texte, sizeof(g_ui_start_bar_texte), value);
}

const char *get_var_fan_1_label()
{
    return g_fan_1_label;
}

void set_var_fan_1_label(const char *value)
{
    set_text(g_fan_1_label, sizeof(g_fan_1_label), value);
}

const char *get_var_fan_1_rpm()
{
    return g_fan_1_rpm_text;
}

void set_var_fan_1_rpm(const char *value)
{
    set_text(g_fan_1_rpm_text, sizeof(g_fan_1_rpm_text), value);
    g_fan_1_rpm = parse_non_negative_int_or_zero(g_fan_1_rpm_text);
}

const char *get_var_fan_2_label()
{
    return g_fan_2_label;
}

void set_var_fan_2_label(const char *value)
{
    set_text(g_fan_2_label, sizeof(g_fan_2_label), value);
}

const char *get_var_fan_2_rmp()
{
    return g_fan_2_rpm_text;
}

void set_var_fan_2_rmp(const char *value)
{
    set_text(g_fan_2_rpm_text, sizeof(g_fan_2_rpm_text), value);
    g_fan_2_rpm = parse_non_negative_int_or_zero(g_fan_2_rpm_text);
}

const char *get_var_fan_2_rpm()
{
    return get_var_fan_2_rmp();
}

void set_var_fan_2_rpm(const char *value)
{
    set_var_fan_2_rmp(value);
}

const char *get_var_fan_3_rpm()
{
    return g_fan_3_rpm_text;
}

void set_var_fan_3_rpm(const char *value)
{
    set_text(g_fan_3_rpm_text, sizeof(g_fan_3_rpm_text), value);
    g_fan_3_rpm = parse_non_negative_int_or_zero(g_fan_3_rpm_text);
}

const char *get_var_fan_3_label()
{
    return g_fan_3_label;
}

void set_var_fan_3_label(const char *value)
{
    set_text(g_fan_3_label, sizeof(g_fan_3_label), value);
}

int32_t get_var_fan_1_pct()
{
    return g_fan_1_pct;
}

void set_var_fan_1_pct(int32_t value)
{
    if (value < 0) {
        g_fan_1_pct = 0;
    } else if (value > 100) {
        g_fan_1_pct = 100;
    } else {
        g_fan_1_pct = value;
    }
}

int32_t get_var_fan_2_pct()
{
    return g_fan_2_pct;
}

void set_var_fan_2_pct(int32_t value)
{
    if (value < 0) {
        g_fan_2_pct = 0;
    } else if (value > 100) {
        g_fan_2_pct = 100;
    } else {
        g_fan_2_pct = value;
    }
}

int32_t get_var_fan_3_pct()
{
    return g_fan_3_pct;
}

void set_var_fan_3_pct(int32_t value)
{
    if (value < 0) {
        g_fan_3_pct = 0;
    } else if (value > 100) {
        g_fan_3_pct = 100;
    } else {
        g_fan_3_pct = value;
    }
}

int32_t vars_get_fan_1_rpm_value(void)
{
    return g_fan_1_rpm;
}

int32_t vars_get_fan_2_rpm_value(void)
{
    return g_fan_2_rpm;
}

int32_t vars_get_fan_3_rpm_value(void)
{
    return g_fan_3_rpm;
}

void vars_get_graph_sample(vars_graph_sample_t *out)
{
    if (out == NULL) {
        return;
    }

    out->cpu_pct = g_cpu_pct_num.value;
    out->cpu_pct_valid = g_cpu_pct_num.valid;
    out->gpu_pct = g_gpu_pct_num.value;
    out->gpu_pct_valid = g_gpu_pct_num.valid;
    out->cpu_temp_c = g_cpu_temp_num.value;
    out->cpu_temp_c_valid = g_cpu_temp_num.valid;
    out->gpu_temp_c = g_gpu_temp_num.value;
    out->gpu_temp_c_valid = g_gpu_temp_num.valid;
}
