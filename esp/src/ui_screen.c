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

static void ui_labels_tick(lv_timer_t *timer)
{
    (void)timer;
    ui_tick();
}

static void ui_graphs_tick(lv_timer_t *timer)
{
    (void)timer;

    vars_graph_sample_t sample = {0};
    vars_get_graph_sample(&sample);
    ui_graphs_push_sample(&sample);
}

void ui_screen_start(void)
{
    if (!s_started) {
        if (objects.main == NULL) {
            ui_init();
        }
        ui_graphs_init(objects.usage_panel, objects.temp_panel);

        s_ui_tick_timer = lv_timer_create(ui_labels_tick, 250, NULL);
        s_graph_timer = lv_timer_create(ui_graphs_tick, 1000, NULL);
        (void)s_ui_tick_timer;
        (void)s_graph_timer;
        s_started = true;
    }

    ui_labels_tick(NULL);
    ui_graphs_tick(NULL);
}
