#include "ui_screen.h"

#include <stdbool.h>
#include <stdio.h>
#include <lvgl.h>

#include "ui/ui.h"
#include "ui/screens.h"
#include "ui_graphs.h"
#include "vars.h"

static lv_timer_t *s_ui_tick_timer;
static lv_timer_t *s_graph_timer;
static bool s_started;
static enum ScreensEnum s_active_screen = SCREEN_ID_MAIN;

static void ui_apply_fan_runtime_patch(void)
{
    if (objects.fan == NULL) {
        return;
    }

    if (objects.fan_1 != NULL) {
        lv_obj_clear_flag(objects.fan_1, LV_OBJ_FLAG_HIDDEN);
    }
    if (objects.fan_4 != NULL) {
        lv_obj_add_flag(objects.fan_4, LV_OBJ_FLAG_HIDDEN);
    }
    if (objects.fan_5 != NULL) {
        lv_obj_add_flag(objects.fan_5, LV_OBJ_FLAG_HIDDEN);
    }
    if (objects.fan_6 != NULL) {
        lv_obj_add_flag(objects.fan_6, LV_OBJ_FLAG_HIDDEN);
    }

    /* Generated fan screen bindings are inconsistent for fan_2.
       Force runtime values on affected labels after generated tick. */
    if (objects.obj28 != NULL) {
        lv_label_set_text(objects.obj28, get_var_fan_2_label());
    }
    if (objects.obj29 != NULL) {
        char rpm_buf[24];
        snprintf(rpm_buf, sizeof(rpm_buf), "%d", (int)vars_get_fan_2_rpm_value());
        rpm_buf[sizeof(rpm_buf) - 1] = '\0';
        lv_label_set_text(objects.obj29, rpm_buf);
    }
    if (objects.obj26 != NULL) {
        char rpm_buf[24];
        snprintf(rpm_buf, sizeof(rpm_buf), "%d", (int)vars_get_fan_1_rpm_value());
        rpm_buf[sizeof(rpm_buf) - 1] = '\0';
        lv_label_set_text(objects.obj26, rpm_buf);
    }
    if (objects.obj31 != NULL) {
        char rpm_buf[24];
        snprintf(rpm_buf, sizeof(rpm_buf), "%d", (int)vars_get_fan_3_rpm_value());
        rpm_buf[sizeof(rpm_buf) - 1] = '\0';
        lv_label_set_text(objects.obj31, rpm_buf);
    }
}

static void ui_labels_tick(lv_timer_t *timer)
{
    (void)timer;
    tick_screen_by_id(s_active_screen);
    if (s_active_screen == SCREEN_ID_FAN) {
        ui_apply_fan_runtime_patch();
    }
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
    if (objects.fan_4 != NULL) {
        lv_obj_add_flag(objects.fan_4, LV_OBJ_FLAG_HIDDEN);
    }
    if (objects.fan_5 != NULL) {
        lv_obj_add_flag(objects.fan_5, LV_OBJ_FLAG_HIDDEN);
    }
    if (objects.fan_6 != NULL) {
        lv_obj_add_flag(objects.fan_6, LV_OBJ_FLAG_HIDDEN);
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

void ui_screen_set_fans_visibility(bool fan2_visible, bool fan3_visible)
{
    if (objects.fan_1 != NULL) {
        lv_obj_clear_flag(objects.fan_1, LV_OBJ_FLAG_HIDDEN);
    }
    if (objects.fan_2 != NULL) {
        if (fan2_visible) {
            lv_obj_clear_flag(objects.fan_2, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(objects.fan_2, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (objects.fan_3 != NULL) {
        if (fan3_visible) {
            lv_obj_clear_flag(objects.fan_3, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(objects.fan_3, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (objects.fan_4 != NULL) {
        lv_obj_add_flag(objects.fan_4, LV_OBJ_FLAG_HIDDEN);
    }
    if (objects.fan_5 != NULL) {
        lv_obj_add_flag(objects.fan_5, LV_OBJ_FLAG_HIDDEN);
    }
    if (objects.fan_6 != NULL) {
        lv_obj_add_flag(objects.fan_6, LV_OBJ_FLAG_HIDDEN);
    }
}
