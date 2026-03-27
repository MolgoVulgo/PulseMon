#ifndef PULSEMON_UI_GRAPHS_H
#define PULSEMON_UI_GRAPHS_H

#include <lvgl.h>

#include "vars.h"

void ui_graphs_init(lv_obj_t *usage_panel, lv_obj_t *temp_panel);
void ui_graphs_push_sample(const vars_graph_sample_t *sample);

#endif
