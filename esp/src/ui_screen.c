#include "ui_screen.h"

#include <stdbool.h>
#include <lvgl.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "lv_port.h"

#ifndef PULSEMON_DEBUG
#define PULSEMON_DEBUG 0
#endif

#include "ui/ui.h"
#include "ui/screens.h"
#include "ui_graphs.h"
#include "printer_service.h"
#include "printer_thumbnail.h"
#include "vars.h"

#if PULSEMON_DEBUG
static const char *TAG = "UI_SCREEN";
#endif
static lv_timer_t *s_ui_tick_timer;
static lv_timer_t *s_graph_timer;
static bool s_started;
static bool s_printer_was_available;
static bool s_pc_screens_enabled = true;
static enum ScreensEnum s_active_screen = SCREEN_ID_MAIN;
#if PULSEMON_DEBUG
static bool s_transition_diag_pending;
static enum ScreensEnum s_transition_diag_from = SCREEN_ID_MAIN;
static enum ScreensEnum s_transition_diag_to = SCREEN_ID_MAIN;
static int64_t s_transition_diag_start_us;
#endif


static bool screen_is_pc_monitor(enum ScreensEnum screen_id)
{
    return screen_id == SCREEN_ID_MAIN || screen_id == SCREEN_ID_GPU;
}

static bool screen_is_allowed(enum ScreensEnum screen_id)
{
    return s_pc_screens_enabled || !screen_is_pc_monitor(screen_id);
}

static lv_obj_t *screen_object_from_id(enum ScreensEnum screen_id)
{
    switch (screen_id) {
    case SCREEN_ID_MAIN:
        return objects.main;
    case SCREEN_ID_GPU:
        return objects.gpu;
    case SCREEN_ID_METEO:
        return objects.meteo;
    case SCREEN_ID_PRINTER:
        return objects.printer;
    default:
        return NULL;
    }
}

#if PULSEMON_DEBUG
static const char *screen_name(enum ScreensEnum screen_id)
{
    switch (screen_id) {
    case SCREEN_ID_MAIN:
        return "main";
    case SCREEN_ID_GPU:
        return "gpu";
    case SCREEN_ID_METEO:
        return "meteo";
    case SCREEN_ID_PRINTER:
        return "printer";
    default:
        return "unknown";
    }
}

static void ui_transition_loaded_cb(lv_event_t *event)
{
    if (!s_transition_diag_pending || lv_event_get_code(event) != LV_EVENT_SCREEN_LOADED) {
        return;
    }
    lv_obj_t *expected = screen_object_from_id(s_transition_diag_to);
    if (expected == NULL || lv_event_get_target(event) != expected) {
        return;
    }

    lvgl_port_perf_stats_t stats = {0};
    lvgl_port_perf_window_snapshot(&stats);
    int64_t elapsed_us = esp_timer_get_time() - s_transition_diag_start_us;
    uint64_t avg_us = stats.flush_count > 0 ? stats.total_us / stats.flush_count : 0;
    ESP_LOGI(TAG,
             "pulsemon_perf owner=ui stage=transition from=%s to=%s elapsed_us=%lld flushes=%llu flush_total_us=%llu flush_avg_us=%llu flush_max_us=%u pixels=%llu",
             screen_name(s_transition_diag_from),
             screen_name(s_transition_diag_to),
             (long long)elapsed_us,
             (unsigned long long)stats.flush_count,
             (unsigned long long)stats.total_us,
             (unsigned long long)avg_us,
             (unsigned int)stats.max_us,
             (unsigned long long)stats.pixels);
    s_transition_diag_pending = false;
}
#endif

static void ui_reset_printer_values(void)
{
    set_var_name_printer("--");
    set_var_printer_ip("--");
    set_var_print_file_name("--");
    set_var_print_time_start("--:--");
    set_var_print_time_end("--:--");
    set_var_print_time_elapsed("00:00:00");
    set_var_print_time_remaining("00:00:00");
    set_var_print_layer("--/--");
    set_var_print_bar(0);
}

static void ui_update_printer_availability(void)
{
    bool available = printer_service_is_available();
    bool cached = printer_service_has_cached_display();
    bool displayable = available || cached;
    if (!displayable && s_printer_was_available) {
        ui_reset_printer_values();
    }
    s_printer_was_available = displayable;

    if (objects.imp_gone != NULL) {
        if (displayable) {
            lv_obj_add_flag(objects.imp_gone, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_clear_flag(objects.imp_gone, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (objects.image_gode != NULL) {
        if (displayable) {
            lv_obj_clear_flag(objects.image_gode, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(objects.image_gode, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void ui_apply_screen_transition(enum ScreensEnum previous, enum ScreensEnum next)
{
    if (previous == next) {
        if (next == SCREEN_ID_PRINTER) {
            ui_update_printer_availability();
        }
        return;
    }

    if (previous == SCREEN_ID_PRINTER) {
        printer_service_stop();
    }

    if (next == SCREEN_ID_PRINTER) {
        bool restored = printer_service_restore_cached_display();
        if (!restored) {
            ui_reset_printer_values();
        }
        s_printer_was_available = restored;
        ui_update_printer_availability();
        (void)printer_service_start();
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
    if (s_active_screen == SCREEN_ID_PRINTER) {
        ui_update_printer_availability();
    }
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
    if (objects.printer != NULL) {
        lv_obj_clear_flag(objects.printer, LV_OBJ_FLAG_SCROLLABLE);
    }
    if (objects.obj21 != NULL) {
        lv_obj_clear_flag(objects.obj21, LV_OBJ_FLAG_SCROLLABLE);
    }
    if (objects.obj23 != NULL) {
        lv_obj_clear_flag(objects.obj23, LV_OBJ_FLAG_SCROLLABLE);
    }
#if PULSEMON_DEBUG
    if (objects.main != NULL) {
        lv_obj_add_event_cb(objects.main, ui_transition_loaded_cb, LV_EVENT_SCREEN_LOADED, NULL);
    }
    if (objects.gpu != NULL) {
        lv_obj_add_event_cb(objects.gpu, ui_transition_loaded_cb, LV_EVENT_SCREEN_LOADED, NULL);
    }
    if (objects.meteo != NULL) {
        lv_obj_add_event_cb(objects.meteo, ui_transition_loaded_cb, LV_EVENT_SCREEN_LOADED, NULL);
    }
    if (objects.printer != NULL) {
        lv_obj_add_event_cb(objects.printer, ui_transition_loaded_cb, LV_EVENT_SCREEN_LOADED, NULL);
    }
#endif
}

void ui_screen_start(void)
{
    if (!s_started) {
        if (objects.main == NULL) {
            ui_init();
        }

        s_active_screen = SCREEN_ID_MAIN;
        ui_prepare_screen_roots();
        printer_thumbnail_init(objects.image_gode);
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

void ui_screen_show_and_release_start(enum ScreensEnum screen_id)
{
    if (!screen_is_allowed(screen_id)) {
        return;
    }

    lv_obj_t *target = screen_object_from_id(screen_id);
    if (target == NULL) {
        return;
    }

    lv_obj_t *start = objects.start;
    bool release_start = start != NULL && lv_scr_act() == start;
    lv_scr_load_anim(target, LV_SCR_LOAD_ANIM_FADE_IN, 200, 0, release_start);
    enum ScreensEnum previous = s_active_screen;
    s_active_screen = screen_id;
    ui_apply_screen_transition(previous, screen_id);
    if (release_start) {
        objects.start = NULL;
        objects.ui_start_bar = NULL;
        objects.ui_start_bar_texte = NULL;
        objects.obj0 = NULL;
    }
    tick_screen_by_id(screen_id);
}

void ui_screen_show_main_and_release_start(void)
{
    ui_screen_show_and_release_start(SCREEN_ID_MAIN);
}

void ui_screen_note_transition_start(enum ScreensEnum screen_id)
{
#if PULSEMON_DEBUG
    if (screen_id < _SCREEN_ID_FIRST || screen_id > _SCREEN_ID_LAST ||
        screen_id == s_active_screen || screen_object_from_id(screen_id) == NULL) {
        return;
    }
    s_transition_diag_from = s_active_screen;
    s_transition_diag_to = screen_id;
    s_transition_diag_start_us = esp_timer_get_time();
    s_transition_diag_pending = true;
    lvgl_port_perf_window_reset();
#else
    (void)screen_id;
#endif
}

void ui_screen_set_active(enum ScreensEnum screen_id)
{
    if (!screen_is_allowed(screen_id) || screen_object_from_id(screen_id) == NULL) {
        return;
    }

    enum ScreensEnum previous = s_active_screen;
    s_active_screen = screen_id;
    ui_apply_screen_transition(previous, screen_id);
}

void ui_screen_load(enum ScreensEnum screen_id, lv_scr_load_anim_t anim)
{
    if (screen_id < _SCREEN_ID_FIRST || screen_id > _SCREEN_ID_LAST ||
        !screen_is_allowed(screen_id)) {
        return;
    }

    lv_obj_t *target = screen_object_from_id(screen_id);
    if (target == NULL) {
        return;
    }

    enum ScreensEnum previous = s_active_screen;
    if (lv_scr_act() == target) {
        s_active_screen = screen_id;
        ui_apply_screen_transition(previous, screen_id);
        tick_screen_by_id(screen_id);
        return;
    }

    ui_screen_note_transition_start(screen_id);
    lv_scr_load_anim(target, anim, PULSEMON_UI_SCREEN_TRANSITION_MS, 0, false);
    s_active_screen = screen_id;
    ui_apply_screen_transition(previous, screen_id);
    tick_screen_by_id(screen_id);
}

enum ScreensEnum ui_screen_get_active(void)
{
    return s_active_screen;
}

void ui_screen_set_pc_screens_enabled(bool enabled)
{
    s_pc_screens_enabled = enabled;
}

bool ui_screen_pc_screens_enabled(void)
{
    return s_pc_screens_enabled;
}
