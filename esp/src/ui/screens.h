#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl/lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Screens

enum ScreensEnum {
    _SCREEN_ID_FIRST = 1,
    SCREEN_ID_MAIN = 1,
    _SCREEN_ID_LAST = 1
};

typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *cpu;
    lv_obj_t *obj0;
    lv_obj_t *obj1;
    lv_obj_t *cpu_temp;
    lv_obj_t *memory;
    lv_obj_t *obj2;
    lv_obj_t *mem_pct;
    lv_obj_t *obj3;
    lv_obj_t *obj4;
    lv_obj_t *mem_used;
    lv_obj_t *mem_total;
    lv_obj_t *obj5;
    lv_obj_t *obj6;
    lv_obj_t *obj7;
    lv_obj_t *gpu_temp;
    lv_obj_t *obj8;
    lv_obj_t *gpu_power;
    lv_obj_t *obj9;
    lv_obj_t *host_meta;
    lv_obj_t *cpu_pct;
    lv_obj_t *usage_panel;
    lv_obj_t *temp_panel;
} objects_t;

extern objects_t objects;

void create_screen_main();
void tick_screen_main();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/