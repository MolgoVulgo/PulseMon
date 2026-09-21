#include "ui_screen.h"

#include <stdbool.h>
#include <lvgl.h>

#include "ui/ui.h"
#include "ui/screens.h"
#include "ui_graphs.h"
#include "vars.h"

static lv_timer_t *s_ui_tick_timer;
static lv_timer_t *s_graph_timer;
static bool s_started;
static enum ScreensEnum s_active_screen = SCREEN_ID_MAIN;

static lv_obj_t *screen_object_from_id(enum ScreensEnum screen_id)
{
    switch (screen_id) {
    case SCREEN_ID_MAIN:
        return objects.main;
    case SCREEN_ID_GPU:
        return objects.gpu;
    case SCREEN_ID_METEO:
        return objects.meteo;
    default:
        return NULL;
    }
}

static int32_t clamp_start_progress(int32_t pct)
{
    if (pct < 0) {
        return 0;
    }
    if (pct > 100) {
        return 100;
    }
    return pct;
}

static void ui_labels_tick(lv_timer_t *timer)
{
    (void)timer;
    tick_screen_by_id(s_active_screen);
}

static void ui_graphs_tick(lv_timer_t *timer)
{
    (void)timer;

    vars_graph_sample_t sample = {0};
    vars_get_graph_sample(&sample);
    ui_graphs_push_sample(&sample);
}

static void ui_prepare_screen_roots(void)
{
    if (objects.main != NULL) {
        lv_obj_clear_flag(objects.main, LV_OBJ_FLAG_SCROLLABLE);
    }
    if (objects.gpu != NULL) {
        lv_obj_clear_flag(objects.gpu, LV_OBJ_FLAG_SCROLLABLE);
    }
    if (objects.meteo != NULL) {
        lv_obj_clear_flag(objects.meteo, LV_OBJ_FLAG_SCROLLABLE);
    }
    if (objects.obj21 != NULL) {
        lv_obj_clear_flag(objects.obj21, LV_OBJ_FLAG_SCROLLABLE);
    }
    if (objects.obj23 != NULL) {
        lv_obj_clear_flag(objects.obj23, LV_OBJ_FLAG_SCROLLABLE);
    }
}

void ui_screen_start(void)
{
    if (!s_started) {
        if (objects.main == NULL) {
            ui_init();
        }

        s_active_screen = SCREEN_ID_MAIN;
        ui_prepare_screen_roots();
        ui_graphs_init(objects.usage_panel, objects.temp_panel);
        ui_graphs_init_gpu(objects.graph_gpu_pct, objects.graph_gpu_temp);

        s_ui_tick_timer = lv_timer_create(ui_labels_tick, 250, NULL);
        s_graph_timer = lv_timer_create(ui_graphs_tick, 1000, NULL);
        (void)s_ui_tick_timer;
        (void)s_graph_timer;
        s_started = true;
    }

    ui_labels_tick(NULL);
    ui_graphs_tick(NULL);
}

void ui_screen_set_start_progress(int32_t pct, const char *text)
{
    pct = clamp_start_progress(pct);
    set_var_ui_start_bar(pct);
    if (text != NULL) {
        set_var_ui_start_bar_texte(text);
    }

    if (objects.ui_start_bar != NULL) {
        lv_bar_set_value(objects.ui_start_bar, pct, LV_ANIM_ON);
    }
    if (objects.ui_start_bar_texte != NULL) {
        lv_label_set_text(objects.ui_start_bar_texte, get_var_ui_start_bar_texte());
    }
}

void ui_screen_show_main_and_release_start(void)
{
    if (objects.main == NULL) {
        return;
    }

    lv_obj_t *start = objects.start;
    bool release_start = start != NULL && lv_scr_act() == start;
    lv_scr_load_anim(objects.main, LV_SCR_LOAD_ANIM_FADE_IN, 200, 0, release_start);
    s_active_screen = SCREEN_ID_MAIN;
    if (release_start) {
        objects.start = NULL;
        objects.ui_start_bar = NULL;
        objects.ui_start_bar_texte = NULL;
        objects.obj0 = NULL;
    }
    tick_screen_by_id(SCREEN_ID_MAIN);
}

void ui_screen_set_active(enum ScreensEnum screen_id)
{
    if (screen_object_from_id(screen_id) == NULL) {
        return;
    }
    s_active_screen = screen_id;
}

void ui_screen_load(enum ScreensEnum screen_id, lv_scr_load_anim_t anim)
{
    if (screen_id < _SCREEN_ID_FIRST || screen_id > _SCREEN_ID_LAST) {
        return;
    }

    lv_obj_t *target = screen_object_from_id(screen_id);
    if (target == NULL) {
        return;
    }

    if (lv_scr_act() == target) {
        s_active_screen = screen_id;
        tick_screen_by_id(screen_id);
        return;
    }

    lv_scr_load_anim(target, anim, 220, 0, false);
    s_active_screen = screen_id;
    tick_screen_by_id(screen_id);
}

enum ScreensEnum ui_screen_get_active(void)
{
    return s_active_screen;
}
