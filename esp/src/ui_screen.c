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

void ui_screen_set_active(enum ScreensEnum screen_id)
{
    if (screen_id < _SCREEN_ID_FIRST || screen_id > _SCREEN_ID_LAST) {
        return;
    }
    s_active_screen = screen_id;
}

enum ScreensEnum ui_screen_get_active(void)
{
    return s_active_screen;
}
