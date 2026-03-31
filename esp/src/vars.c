#include <ctype.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "vars.h"

#define VAR_BUF_LEN 64
#define HOST_META_BUF_LEN 160

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
