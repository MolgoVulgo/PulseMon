#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl/lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Screens

enum ScreensEnum {
    _SCREEN_ID_FIRST = 1,
    SCREEN_ID_START = 1,
    SCREEN_ID_MAIN = 2,
    SCREEN_ID_GPU = 3,
    SCREEN_ID_FAN = 4,
    SCREEN_ID_METEO = 5,
    _SCREEN_ID_LAST = 5
};

typedef struct _objects_t {
    lv_obj_t *start;
    lv_obj_t *main;
    lv_obj_t *gpu;
    lv_obj_t *fan;
    lv_obj_t *meteo;
    lv_obj_t *ui_start_bar;
    lv_obj_t *ui_start_bar_texte;
    lv_obj_t *obj0;
    lv_obj_t *cpu;
    lv_obj_t *obj1;
    lv_obj_t *cpu_temp_2;
    lv_obj_t *obj2;
    lv_obj_t *cpu_temp_1;
    lv_obj_t *cpu_pct_1;
    lv_obj_t *cpu_pct_2;
    lv_obj_t *memory;
    lv_obj_t *obj3;
    lv_obj_t *mem_pct_1;
    lv_obj_t *obj4;
    lv_obj_t *obj5;
    lv_obj_t *mem_used;
    lv_obj_t *mem_total;
    lv_obj_t *obj6;
    lv_obj_t *mem_pct;
    lv_obj_t *mem_used_1;
    lv_obj_t *mem_total_1;
    lv_obj_t *obj7;
    lv_obj_t *obj8;
    lv_obj_t *obj9;
    lv_obj_t *gpu_temp;
    lv_obj_t *obj10;
    lv_obj_t *gpu_power;
    lv_obj_t *gpu_pct;
    lv_obj_t *obj11;
    lv_obj_t *gpu_pct_2;
    lv_obj_t *gpu_temp_2;
    lv_obj_t *gpu_power_2;
    lv_obj_t *obj12;
    lv_obj_t *host_meta;
    lv_obj_t *usage_panel;
    lv_obj_t *temp_panel;
    lv_obj_t *obj13;
    lv_obj_t *obj14;
    lv_obj_t *gpu_pct_1;
    lv_obj_t *gpu_pct_3;
    lv_obj_t *obj15;
    lv_obj_t *host_meta_1;
    lv_obj_t *graph_gpu_pct;
    lv_obj_t *obj16;
    lv_obj_t *obj17;
    lv_obj_t *gpu_temp_1;
    lv_obj_t *gpu_temp_;
    lv_obj_t *obj18;
    lv_obj_t *obj19;
    lv_obj_t *gpu_power_1;
    lv_obj_t *gpu_power_3;
    lv_obj_t *obj20;
    lv_obj_t *obj21;
    lv_obj_t *gpu_pct_4;
    lv_obj_t *gpu_pct_5;
    lv_obj_t *gpu_pct_6;
    lv_obj_t *gpu_pct_7;
    lv_obj_t *obj22;
    lv_obj_t *obj23;
    lv_obj_t *obj24;
    lv_obj_t *obj25;
    lv_obj_t *gpu_vram_total;
    lv_obj_t *graph_gpu_temp;
    lv_obj_t *obj26;
    lv_obj_t *fan_1;
    lv_obj_t *fan_1_pct;
    lv_obj_t *obj27;
    lv_obj_t *obj28;
    lv_obj_t *fan_2;
    lv_obj_t *obj29;
    lv_obj_t *obj30;
    lv_obj_t *obj31;
    lv_obj_t *fan_3;
    lv_obj_t *obj32;
    lv_obj_t *obj33;
    lv_obj_t *obj34;
    lv_obj_t *fan_host_meta;
    lv_obj_t *obj35;
    lv_obj_t *obj36;
    lv_obj_t *obj37;
    lv_obj_t *fan_4;
    lv_obj_t *obj38;
    lv_obj_t *obj39;
    lv_obj_t *fan_5;
    lv_obj_t *obj40;
    lv_obj_t *obj41;
    lv_obj_t *fan_6;
    lv_obj_t *obj42;
    lv_obj_t *obj43;
    lv_obj_t *ui_meteo_clock;
    lv_obj_t *ui_meteo_img;
    lv_obj_t *ui_meteo_date;
    lv_obj_t *ui_meteo_temp;
    lv_obj_t *ui_meteo_condition;
    lv_obj_t *obj44;
    lv_obj_t *ui_meteo_fi1;
    lv_obj_t *ui_meteo_fi2;
    lv_obj_t *ui_meteo_fi3;
    lv_obj_t *ui_meteo_fi4;
    lv_obj_t *ui_meteo_fi5;
    lv_obj_t *ui_meteo_fi6;
    lv_obj_t *ui_meteo_ft1_1;
    lv_obj_t *obj45;
    lv_obj_t *ui_meteo_fd1;
    lv_obj_t *ui_meteo_fd2;
    lv_obj_t *ui_meteo_fd3;
    lv_obj_t *ui_meteo_fd4;
    lv_obj_t *ui_meteo_fd5;
    lv_obj_t *ui_meteo_fd6;
    lv_obj_t *obj46;
    lv_obj_t *obj47;
    lv_obj_t *obj48;
    lv_obj_t *obj49;
    lv_obj_t *obj50;
    lv_obj_t *ui_meteo_ft1_2;
    lv_obj_t *ui_meteo_ft1_3;
    lv_obj_t *ui_meteo_ft1_4;
    lv_obj_t *ui_meteo_ft1_5;
    lv_obj_t *ui_meteo_ft1_6;
    lv_obj_t *obj51;
    lv_obj_t *aler_meteo;
    lv_obj_t *meteo_alert;
} objects_t;

extern objects_t objects;

void create_screen_start();
void tick_screen_start();

void create_screen_main();
void tick_screen_main();

void create_screen_gpu();
void tick_screen_gpu();

void create_screen_fan();
void tick_screen_fan();

void create_screen_meteo();
void tick_screen_meteo();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/