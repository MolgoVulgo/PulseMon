#include "ui_graphs.h"

#include <stdbool.h>

static lv_obj_t *s_usage_chart;
static lv_obj_t *s_temp_chart;
static lv_chart_series_t *s_usage_cpu_series;
static lv_chart_series_t *s_usage_gpu_series;
static lv_chart_series_t *s_temp_cpu_series;
static lv_chart_series_t *s_temp_gpu_series;
static bool s_initialized;

static void init_chart_common(lv_obj_t *chart)
{
    lv_obj_set_size(chart, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(chart, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(chart, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(chart, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_SHIFT);
    lv_chart_set_div_line_count(chart, 4, 6);
    lv_chart_set_point_count(chart, 90);
}

static int16_t chart_value_or_none(float value, bool valid)
{
    if (!valid) {
        return LV_CHART_POINT_NONE;
    }
    return (int16_t)value;
}

void ui_graphs_init(lv_obj_t *usage_panel, lv_obj_t *temp_panel)
{
    if (s_initialized || usage_panel == NULL || temp_panel == NULL) {
        return;
    }

    s_usage_chart = lv_chart_create(usage_panel);
    init_chart_common(s_usage_chart);
    lv_chart_set_range(s_usage_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    s_usage_cpu_series = lv_chart_add_series(s_usage_chart, lv_palette_main(LV_PALETTE_LIGHT_BLUE), LV_CHART_AXIS_PRIMARY_Y);
    s_usage_gpu_series = lv_chart_add_series(s_usage_chart, lv_palette_main(LV_PALETTE_ORANGE), LV_CHART_AXIS_PRIMARY_Y);

    s_temp_chart = lv_chart_create(temp_panel);
    init_chart_common(s_temp_chart);
    lv_chart_set_range(s_temp_chart, LV_CHART_AXIS_PRIMARY_Y, 20, 100);
    s_temp_cpu_series = lv_chart_add_series(s_temp_chart, lv_palette_main(LV_PALETTE_CYAN), LV_CHART_AXIS_PRIMARY_Y);
    s_temp_gpu_series = lv_chart_add_series(s_temp_chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);

    for (uint16_t i = 0; i < lv_chart_get_point_count(s_usage_chart); ++i) {
        s_usage_cpu_series->y_points[i] = LV_CHART_POINT_NONE;
        s_usage_gpu_series->y_points[i] = LV_CHART_POINT_NONE;
        s_temp_cpu_series->y_points[i] = LV_CHART_POINT_NONE;
        s_temp_gpu_series->y_points[i] = LV_CHART_POINT_NONE;
    }
    lv_chart_refresh(s_usage_chart);
    lv_chart_refresh(s_temp_chart);

    s_initialized = true;
}

void ui_graphs_push_sample(const vars_graph_sample_t *sample)
{
    if (!s_initialized || sample == NULL) {
        return;
    }

    lv_chart_set_next_value(s_usage_chart, s_usage_cpu_series, chart_value_or_none(sample->cpu_pct, sample->cpu_pct_valid));
    lv_chart_set_next_value(s_usage_chart, s_usage_gpu_series, chart_value_or_none(sample->gpu_pct, sample->gpu_pct_valid));

    lv_chart_set_next_value(s_temp_chart, s_temp_cpu_series, chart_value_or_none(sample->cpu_temp_c, sample->cpu_temp_c_valid));
    lv_chart_set_next_value(s_temp_chart, s_temp_gpu_series, chart_value_or_none(sample->gpu_temp_c, sample->gpu_temp_c_valid));
}
