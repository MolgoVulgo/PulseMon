#include <lvgl.h>

#include "ui_graphs.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define GRAPH_POINT_COUNT 90
#define AXIS_LABEL_COUNT 5
#define USAGE_RANGE_MIN 0
#define USAGE_RANGE_MAX 100
#define TEMP_RANGE_MIN 20
#define TEMP_RANGE_MAX 90

static lv_obj_t *s_usage_chart;
static lv_obj_t *s_temp_chart;
static lv_chart_series_t *s_usage_cpu_series;
static lv_chart_series_t *s_usage_gpu_series;
static lv_chart_series_t *s_temp_cpu_series;
static lv_chart_series_t *s_temp_gpu_series;
static lv_obj_t *s_usage_axis_labels[AXIS_LABEL_COUNT];
static lv_obj_t *s_temp_axis_labels[AXIS_LABEL_COUNT];
static lv_coord_t s_usage_cpu_points[GRAPH_POINT_COUNT];
static lv_coord_t s_usage_gpu_points[GRAPH_POINT_COUNT];
static lv_coord_t s_temp_cpu_points[GRAPH_POINT_COUNT];
static lv_coord_t s_temp_gpu_points[GRAPH_POINT_COUNT];
static bool s_initialized;

static void set_text_if_changed(lv_obj_t *label, const char *next_text)
{
    if (label == NULL) {
        return;
    }
    const char *current = lv_label_get_text(label);
    const char *next = (next_text != NULL) ? next_text : "";
    if (current != NULL && strcmp(current, next) == 0) {
        return;
    }
    lv_label_set_text(label, next);
}

static lv_obj_t *create_axis_value_label(lv_obj_t *parent)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_style_text_color(label, lv_color_hex(0x90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(label, "-");
    return label;
}

static void update_chart_axis_labels(lv_obj_t *chart, lv_obj_t *labels[AXIS_LABEL_COUNT], int range_min, int range_max)
{
    if (chart == NULL) {
        return;
    }

    /* Ensure width/height are final before computing Y positions. */
    lv_obj_update_layout(chart);

    lv_coord_t chart_x = lv_obj_get_x(chart);
    lv_coord_t chart_y = lv_obj_get_y(chart);
    lv_coord_t chart_h = lv_obj_get_height(chart);
    if (chart_h < 1) {
        chart_h = 1;
    }
    lv_coord_t pad_top = lv_obj_get_style_pad_top(chart, LV_PART_MAIN);
    lv_coord_t pad_bottom = lv_obj_get_style_pad_bottom(chart, LV_PART_MAIN);
    lv_coord_t border = lv_obj_get_style_border_width(chart, LV_PART_MAIN);
    lv_coord_t plot_top = chart_y + pad_top + border;
    lv_coord_t plot_h = chart_h - pad_top - pad_bottom - (border * 2);
    if (plot_h < 1) {
        plot_h = chart_h;
        plot_top = chart_y;
    }
    lv_coord_t label_w = (chart_x > 4) ? (chart_x - 4) : 10;

    char buf[12];
    float span = (float)(range_max - range_min);
    for (size_t i = 0; i < AXIS_LABEL_COUNT; ++i) {
        lv_obj_t *label = labels[i];
        if (label == NULL) {
            continue;
        }
        float ratio = (AXIS_LABEL_COUNT > 1) ? ((float)i / (float)(AXIS_LABEL_COUNT - 1)) : 0.0f;
        float value = (float)range_max - (span * ratio);
        snprintf(buf, sizeof(buf), "%.0f", (double)value);
        set_text_if_changed(label, buf);

        lv_obj_set_width(label, label_w);
        lv_obj_update_layout(label);
        lv_coord_t y = plot_top + (lv_coord_t)((float)(plot_h - 1) * ratio) - (lv_obj_get_height(label) / 2);
        lv_obj_set_pos(label, 0, y);
    }
}

static void init_chart_common(lv_obj_t *chart)
{
    lv_obj_set_pos(chart, 30, 18);
    lv_obj_set_size(chart, 186, 114);
    lv_obj_clear_flag(chart, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(chart, lv_color_hex(0x0f131d), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(chart, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(chart, lv_color_hex(0x232a38), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(chart, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(chart, lv_color_hex(0x2a3140), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(chart, lv_color_hex(0x90a0bc), LV_PART_TICKS | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(chart, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(chart, 2, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_size(chart, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_SHIFT);
    lv_chart_set_div_line_count(chart, 5, 6);
    lv_chart_set_point_count(chart, GRAPH_POINT_COUNT);
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
    lv_obj_clear_flag(usage_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(temp_panel, LV_OBJ_FLAG_SCROLLABLE);

    s_usage_chart = lv_chart_create(usage_panel);
    init_chart_common(s_usage_chart);
    lv_chart_set_range(s_usage_chart, LV_CHART_AXIS_PRIMARY_Y, USAGE_RANGE_MIN, USAGE_RANGE_MAX);
    s_usage_cpu_series = lv_chart_add_series(s_usage_chart, lv_color_hex(0x62a0ea), LV_CHART_AXIS_PRIMARY_Y);
    s_usage_gpu_series = lv_chart_add_series(s_usage_chart, lv_color_hex(0xf66151), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_ext_y_array(s_usage_chart, s_usage_cpu_series, s_usage_cpu_points);
    lv_chart_set_ext_y_array(s_usage_chart, s_usage_gpu_series, s_usage_gpu_points);
    for (size_t i = 0; i < AXIS_LABEL_COUNT; ++i) {
        s_usage_axis_labels[i] = create_axis_value_label(usage_panel);
    }
    update_chart_axis_labels(s_usage_chart, s_usage_axis_labels, USAGE_RANGE_MIN, USAGE_RANGE_MAX);

    s_temp_chart = lv_chart_create(temp_panel);
    init_chart_common(s_temp_chart);
    lv_chart_set_range(s_temp_chart, LV_CHART_AXIS_PRIMARY_Y, TEMP_RANGE_MIN, TEMP_RANGE_MAX);
    s_temp_cpu_series = lv_chart_add_series(s_temp_chart, lv_color_hex(0xffb84d), LV_CHART_AXIS_PRIMARY_Y);
    s_temp_gpu_series = lv_chart_add_series(s_temp_chart, lv_color_hex(0xf66151), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_ext_y_array(s_temp_chart, s_temp_cpu_series, s_temp_cpu_points);
    lv_chart_set_ext_y_array(s_temp_chart, s_temp_gpu_series, s_temp_gpu_points);
    for (size_t i = 0; i < AXIS_LABEL_COUNT; ++i) {
        s_temp_axis_labels[i] = create_axis_value_label(temp_panel);
    }
    update_chart_axis_labels(s_temp_chart, s_temp_axis_labels, TEMP_RANGE_MIN, TEMP_RANGE_MAX);

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
    update_chart_axis_labels(s_usage_chart, s_usage_axis_labels, USAGE_RANGE_MIN, USAGE_RANGE_MAX);
    update_chart_axis_labels(s_temp_chart, s_temp_axis_labels, TEMP_RANGE_MIN, TEMP_RANGE_MAX);
    lv_chart_refresh(s_usage_chart);
    lv_chart_refresh(s_temp_chart);
}
