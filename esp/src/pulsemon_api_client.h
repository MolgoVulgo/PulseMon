#ifndef PULSEMON_API_CLIENT_H
#define PULSEMON_API_CLIENT_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    float cpu_pct;
    bool cpu_pct_valid;
    float cpu_temp_c;
    bool cpu_temp_c_valid;
    float mem_pct;
    bool mem_pct_valid;
    unsigned long long mem_used_b;
    bool mem_used_b_valid;
    unsigned long long mem_total_b;
    bool mem_total_b_valid;
    float gpu_pct;
    bool gpu_pct_valid;
    float gpu_temp_c;
    bool gpu_temp_c_valid;
    float gpu_power_w;
    bool gpu_power_w_valid;
    bool state_ok;
    bool state_ok_valid;
    long stale_ms;
    bool stale_ms_valid;
    char host[96];
    bool host_valid;
} pulsemon_dashboard_t;

bool pulsemon_fetch_dashboard(pulsemon_dashboard_t *out, char *err, size_t err_len);

typedef struct {
    float pct;
    bool pct_valid;
    float core_clock_mhz;
    bool core_clock_mhz_valid;
    float mem_clock_mhz;
    bool mem_clock_mhz_valid;
    unsigned long long vram_used_b;
    bool vram_used_b_valid;
    unsigned long long vram_total_b;
    bool vram_total_b_valid;
    float vram_pct;
    bool vram_pct_valid;
    float temp_c;
    bool temp_c_valid;
    float power_w;
    bool power_w_valid;
    float fan_rpm;
    bool fan_rpm_valid;
    float fan_pct;
    bool fan_pct_valid;
} pulsemon_gpu_dashboard_t;

bool pulsemon_fetch_gpu_dashboard(pulsemon_gpu_dashboard_t *out, char *err, size_t err_len);

#endif
