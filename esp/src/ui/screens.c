#include <string.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

#include <string.h>

objects_t objects;

//
// Event handlers
//

lv_obj_t *tick_value_change_obj;

static void event_handler_cb_fan_fan_1_pct(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            int32_t value = lv_arc_get_value(ta);
            set_var_fan_1_pct(value);
        }
    }
}

static void event_handler_cb_fan_obj29(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            int32_t value = lv_arc_get_value(ta);
            set_var_fan_2_pct(value);
        }
    }
}

static void event_handler_cb_fan_obj32(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            int32_t value = lv_arc_get_value(ta);
            set_var_fan_3_pct(value);
        }
    }
}

//
// Screens
//

void create_screen_main() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.main = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_add_event_cb(obj, action_ui_swipe, LV_EVENT_GESTURE, (void *)0);
    add_style_defaut(obj);
    {
        lv_obj_t *parent_obj = obj;
        {
            // cpu
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.cpu = obj;
            lv_obj_set_pos(obj, 6, 34);
            lv_obj_set_size(obj, 154, 97);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x171a21), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x2a3140), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj0 = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "CPU");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj1 = obj;
                    lv_obj_set_pos(obj, 0, 40);
                    lv_obj_set_size(obj, 45, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "Temp");
                }
                {
                    // cpu_temp
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.cpu_temp = obj;
                    lv_obj_set_pos(obj, 53, 40);
                    lv_obj_set_size(obj, 40, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xd8deea), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj2 = obj;
                    lv_obj_set_pos(obj, 0, 24);
                    lv_obj_set_size(obj, 45, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "Usage");
                }
            }
        }
        {
            // memory
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.memory = obj;
            lv_obj_set_pos(obj, 163, 34);
            lv_obj_set_size(obj, 154, 97);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x171a21), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x2a3140), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj3 = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "Memory");
                }
                {
                    // mem_pct
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.mem_pct = obj;
                    lv_obj_set_pos(obj, 84, 24);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xd8deea), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj4 = obj;
                    lv_obj_set_pos(obj, 0, 40);
                    lv_obj_set_size(obj, 45, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "Used");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj5 = obj;
                    lv_obj_set_pos(obj, 0, 57);
                    lv_obj_set_size(obj, 45, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "Total");
                }
                {
                    // mem_used
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.mem_used = obj;
                    lv_obj_set_pos(obj, 51, 40);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xd8deea), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
                {
                    // mem_total
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.mem_total = obj;
                    lv_obj_set_pos(obj, 51, 57);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xd8deea), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj6 = obj;
                    lv_obj_set_pos(obj, -1, 24);
                    lv_obj_set_size(obj, 45, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "Usage");
                }
            }
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.obj7 = obj;
            lv_obj_set_pos(obj, 320, 34);
            lv_obj_set_size(obj, 154, 97);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x171a21), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x2a3140), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj8 = obj;
                    lv_obj_set_pos(obj, -4, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "GPU\n");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj9 = obj;
                    lv_obj_set_pos(obj, -4, 40);
                    lv_obj_set_size(obj, 46, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "Temp");
                }
                {
                    // gpu_temp
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.gpu_temp = obj;
                    lv_obj_set_pos(obj, 60, 40);
                    lv_obj_set_size(obj, 50, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xd8deea), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj10 = obj;
                    lv_obj_set_pos(obj, -4, 57);
                    lv_obj_set_size(obj, 46, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "Power");
                }
                {
                    // gpu_power
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.gpu_power = obj;
                    lv_obj_set_pos(obj, 60, 57);
                    lv_obj_set_size(obj, 50, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xd8deea), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
                {
                    // gpu_pct
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.gpu_pct = obj;
                    lv_obj_set_pos(obj, 60, 24);
                    lv_obj_set_size(obj, 50, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xd8deea), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj11 = obj;
                    lv_obj_set_pos(obj, -4, 24);
                    lv_obj_set_size(obj, 46, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "Usage");
                }
            }
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj12 = obj;
            lv_obj_set_pos(obj, 12, 8);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xd8deea), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_font_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "Stats Linux Monitor");
        }
        {
            // host_meta
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.host_meta = obj;
            lv_obj_set_pos(obj, 240, 12);
            lv_obj_set_size(obj, 228, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0x90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // cpu_pct
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.cpu_pct = obj;
            lv_obj_set_pos(obj, 68, 67);
            lv_obj_set_size(obj, 40, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xd8deea), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // usage_panel
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.usage_panel = obj;
            lv_obj_set_pos(obj, 6, 135);
            lv_obj_set_size(obj, 232, 160);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x0f131d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x232a38), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // temp_panel
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.temp_panel = obj;
            lv_obj_set_pos(obj, 242, 135);
            lv_obj_set_size(obj, 232, 160);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x0f131d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x232a38), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
    
    tick_screen_main();
}

void tick_screen_main() {
    {
        const char *new_val = get_var_cpu_temp();
        const char *cur_val = lv_label_get_text(objects.cpu_temp);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.cpu_temp;
            lv_label_set_text(objects.cpu_temp, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_mem_pct();
        const char *cur_val = lv_label_get_text(objects.mem_pct);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.mem_pct;
            lv_label_set_text(objects.mem_pct, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_mem_used();
        const char *cur_val = lv_label_get_text(objects.mem_used);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.mem_used;
            lv_label_set_text(objects.mem_used, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_mem_total();
        const char *cur_val = lv_label_get_text(objects.mem_total);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.mem_total;
            lv_label_set_text(objects.mem_total, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_gpu_temp();
        const char *cur_val = lv_label_get_text(objects.gpu_temp);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.gpu_temp;
            lv_label_set_text(objects.gpu_temp, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_gpu_power();
        const char *cur_val = lv_label_get_text(objects.gpu_power);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.gpu_power;
            lv_label_set_text(objects.gpu_power, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_gpu_pct();
        const char *cur_val = lv_label_get_text(objects.gpu_pct);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.gpu_pct;
            lv_label_set_text(objects.gpu_pct, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_host_meta();
        const char *cur_val = lv_label_get_text(objects.host_meta);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.host_meta;
            lv_label_set_text(objects.host_meta, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_cpu_pct();
        const char *cur_val = lv_label_get_text(objects.cpu_pct);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.cpu_pct;
            lv_label_set_text(objects.cpu_pct, new_val);
            tick_value_change_obj = NULL;
        }
    }
}

void create_screen_gpu() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.gpu = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_add_event_cb(obj, action_ui_swipe, LV_EVENT_GESTURE, (void *)0);
    add_style_defaut(obj);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.obj13 = obj;
            lv_obj_set_pos(obj, 6, 34);
            lv_obj_set_size(obj, 153, 50);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x171a21), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x2a3140), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj14 = obj;
                    lv_obj_set_pos(obj, -4, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "GPU Usage");
                }
                {
                    // gpu_pct_1
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.gpu_pct_1 = obj;
                    lv_obj_set_pos(obj, -4, 16);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xd8deea), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
            }
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj15 = obj;
            lv_obj_set_pos(obj, 12, 8);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xd8deea), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_font_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "Stats Linux Monitor [");
        }
        {
            // host_meta_1
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.host_meta_1 = obj;
            lv_obj_set_pos(obj, 240, 8);
            lv_obj_set_size(obj, 228, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0x90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // graph_gpu_pct
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.graph_gpu_pct = obj;
            lv_obj_set_pos(obj, 6, 141);
            lv_obj_set_size(obj, 232, 160);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x0f131d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x232a38), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.obj16 = obj;
            lv_obj_set_pos(obj, 163, 34);
            lv_obj_set_size(obj, 154, 50);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x171a21), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x2a3140), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj17 = obj;
                    lv_obj_set_pos(obj, -4, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "Temperature");
                }
                {
                    // gpu_temp_1
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.gpu_temp_1 = obj;
                    lv_obj_set_pos(obj, -4, 16);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xd8deea), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
            }
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.obj18 = obj;
            lv_obj_set_pos(obj, 321, 34);
            lv_obj_set_size(obj, 153, 50);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x171a21), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x2a3140), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj19 = obj;
                    lv_obj_set_pos(obj, -4, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "Power");
                }
                {
                    // gpu_power_1
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.gpu_power_1 = obj;
                    lv_obj_set_pos(obj, -4, 16);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xd8deea), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
            }
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.obj20 = obj;
            lv_obj_set_pos(obj, 163, 88);
            lv_obj_set_size(obj, 153, 50);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x171a21), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x2a3140), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj21 = obj;
                    lv_obj_set_pos(obj, -4, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "Fan");
                }
                {
                    // gpu_pct_7
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.gpu_pct_7 = obj;
                    lv_obj_set_pos(obj, -4, 16);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xd8deea), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
            }
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj22 = obj;
            lv_obj_set_pos(obj, 202, 8);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff6e39), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_font_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "GPU");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj23 = obj;
            lv_obj_set_pos(obj, 231, 8);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xd8deea), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_font_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "]");
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.obj24 = obj;
            lv_obj_set_pos(obj, 6, 87);
            lv_obj_set_size(obj, 153, 50);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x171a21), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x2a3140), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj25 = obj;
                    lv_obj_set_pos(obj, -4, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "VRAM");
                }
                {
                    // gpu_vram_total
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.gpu_vram_total = obj;
                    lv_obj_set_pos(obj, 65, 0);
                    lv_obj_set_size(obj, 70, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xd8deea), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
                {
                    lv_obj_t *obj = lv_bar_create(parent_obj);
                    objects.obj26 = obj;
                    lv_obj_set_pos(obj, -4, 17);
                    lv_obj_set_size(obj, 139, 15);
                }
            }
        }
        {
            // graph_gpu_temp
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.graph_gpu_temp = obj;
            lv_obj_set_pos(obj, 242, 141);
            lv_obj_set_size(obj, 232, 160);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x0f131d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x232a38), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
    
    tick_screen_gpu();
}

void tick_screen_gpu() {
    {
        const char *new_val = get_var_gpu_pct();
        const char *cur_val = lv_label_get_text(objects.gpu_pct_1);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.gpu_pct_1;
            lv_label_set_text(objects.gpu_pct_1, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_host_meta();
        const char *cur_val = lv_label_get_text(objects.host_meta_1);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.host_meta_1;
            lv_label_set_text(objects.host_meta_1, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_gpu_temp();
        const char *cur_val = lv_label_get_text(objects.gpu_temp_1);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.gpu_temp_1;
            lv_label_set_text(objects.gpu_temp_1, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_gpu_power();
        const char *cur_val = lv_label_get_text(objects.gpu_power_1);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.gpu_power_1;
            lv_label_set_text(objects.gpu_power_1, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_gpu_fan_rpm();
        const char *cur_val = lv_label_get_text(objects.gpu_pct_7);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.gpu_pct_7;
            lv_label_set_text(objects.gpu_pct_7, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_gpu_vram_total();
        const char *cur_val = lv_label_get_text(objects.gpu_vram_total);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.gpu_vram_total;
            lv_label_set_text(objects.gpu_vram_total, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        int32_t new_val = get_var_gpu_vram_used();
        int32_t cur_val = lv_bar_get_value(objects.obj26);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.obj26;
            lv_bar_set_value(objects.obj26, new_val, LV_ANIM_OFF);
            tick_value_change_obj = NULL;
        }
    }
}

void create_screen_fan() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.fan = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_add_event_cb(obj, action_ui_swipe, LV_EVENT_GESTURE, (void *)0);
    add_style_defaut(obj);
    {
        lv_obj_t *parent_obj = obj;
        {
            // fan_1
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.fan_1 = obj;
            lv_obj_set_pos(obj, 6, 36);
            lv_obj_set_size(obj, 154, 105);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x171a21), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x2a3140), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // fan_1_pct
                    lv_obj_t *obj = lv_arc_create(parent_obj);
                    objects.fan_1_pct = obj;
                    lv_obj_set_pos(obj, 23, -4);
                    lv_obj_set_size(obj, 91, 92);
                    lv_obj_add_event_cb(obj, event_handler_cb_fan_fan_1_pct, LV_EVENT_ALL, 0);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                    lv_obj_set_style_opa(obj, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_KNOB | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x212121), LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj27 = obj;
                    lv_obj_set_pos(obj, 9, 76);
                    lv_obj_set_size(obj, 120, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj28 = obj;
                    lv_obj_set_pos(obj, 53, 34);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
            }
        }
        {
            // fan_2
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.fan_2 = obj;
            lv_obj_set_pos(obj, 163, 36);
            lv_obj_set_size(obj, 154, 105);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x171a21), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x2a3140), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_arc_create(parent_obj);
                    objects.obj29 = obj;
                    lv_obj_set_pos(obj, 23, -3);
                    lv_obj_set_size(obj, 91, 92);
                    lv_obj_add_event_cb(obj, event_handler_cb_fan_obj29, LV_EVENT_ALL, 0);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                    lv_obj_set_style_opa(obj, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_KNOB | LV_STATE_DEFAULT);
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj30 = obj;
                    lv_obj_set_pos(obj, 9, 76);
                    lv_obj_set_size(obj, 120, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "Fan_2");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj31 = obj;
                    lv_obj_set_pos(obj, 53, 37);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
            }
        }
        {
            // fan_3
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.fan_3 = obj;
            lv_obj_set_pos(obj, 320, 36);
            lv_obj_set_size(obj, 154, 105);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x171a21), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x2a3140), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_arc_create(parent_obj);
                    objects.obj32 = obj;
                    lv_obj_set_pos(obj, 23, -1);
                    lv_obj_set_size(obj, 91, 92);
                    lv_obj_add_event_cb(obj, event_handler_cb_fan_obj32, LV_EVENT_ALL, 0);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                    lv_obj_set_style_opa(obj, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_KNOB | LV_STATE_DEFAULT);
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj33 = obj;
                    lv_obj_set_pos(obj, 53, 34);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj34 = obj;
                    lv_obj_set_pos(obj, 8, 76);
                    lv_obj_set_size(obj, 120, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
            }
        }
        {
            // fan_host_meta
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.fan_host_meta = obj;
            lv_obj_set_pos(obj, 240, 12);
            lv_obj_set_size(obj, 228, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0x90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj35 = obj;
            lv_obj_set_pos(obj, 12, 8);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xd8deea), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_font_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "Stats Linux Monitor [");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj36 = obj;
            lv_obj_set_pos(obj, 200, 9);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff6e39), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_font_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "FAN");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj37 = obj;
            lv_obj_set_pos(obj, 226, 8);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xd8deea), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_font_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "]");
        }
        {
            // fan_4
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.fan_4 = obj;
            lv_obj_set_pos(obj, 6, 150);
            lv_obj_set_size(obj, 154, 105);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x171a21), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x2a3140), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_arc_create(parent_obj);
                    lv_obj_set_pos(obj, 23, -4);
                    lv_obj_set_size(obj, 91, 92);
                    lv_arc_set_value(obj, 50);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                    lv_obj_set_style_opa(obj, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_KNOB | LV_STATE_DEFAULT);
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj38 = obj;
                    lv_obj_set_pos(obj, 9, 76);
                    lv_obj_set_size(obj, 120, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj39 = obj;
                    lv_obj_set_pos(obj, 53, 34);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
            }
        }
        {
            // fan_5
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.fan_5 = obj;
            lv_obj_set_pos(obj, 163, 150);
            lv_obj_set_size(obj, 154, 105);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x171a21), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x2a3140), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_arc_create(parent_obj);
                    lv_obj_set_pos(obj, 23, -4);
                    lv_obj_set_size(obj, 91, 92);
                    lv_arc_set_value(obj, 50);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                    lv_obj_set_style_opa(obj, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_KNOB | LV_STATE_DEFAULT);
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj40 = obj;
                    lv_obj_set_pos(obj, 9, 76);
                    lv_obj_set_size(obj, 120, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj41 = obj;
                    lv_obj_set_pos(obj, 53, 34);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
            }
        }
        {
            // fan_6
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.fan_6 = obj;
            lv_obj_set_pos(obj, 320, 150);
            lv_obj_set_size(obj, 154, 105);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x171a21), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x2a3140), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_arc_create(parent_obj);
                    lv_obj_set_pos(obj, 23, -4);
                    lv_obj_set_size(obj, 91, 92);
                    lv_arc_set_value(obj, 50);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                    lv_obj_set_style_opa(obj, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_KNOB | LV_STATE_DEFAULT);
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj42 = obj;
                    lv_obj_set_pos(obj, 9, 76);
                    lv_obj_set_size(obj, 120, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj43 = obj;
                    lv_obj_set_pos(obj, 53, 34);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
            }
        }
    }
    
    tick_screen_fan();
}

void tick_screen_fan() {
    {
        int32_t new_val = get_var_fan_1_pct();
        int32_t cur_val = lv_arc_get_value(objects.fan_1_pct);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.fan_1_pct;
            lv_arc_set_value(objects.fan_1_pct, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_fan_1_label();
        const char *cur_val = lv_label_get_text(objects.obj27);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj27;
            lv_label_set_text(objects.obj27, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_fan_1_rpm();
        const char *cur_val = lv_label_get_text(objects.obj28);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj28;
            lv_label_set_text(objects.obj28, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        int32_t new_val = get_var_fan_2_pct();
        int32_t cur_val = lv_arc_get_value(objects.obj29);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.obj29;
            lv_arc_set_value(objects.obj29, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_fan_1_rpm();
        const char *cur_val = lv_label_get_text(objects.obj31);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj31;
            lv_label_set_text(objects.obj31, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        int32_t new_val = get_var_fan_3_pct();
        int32_t cur_val = lv_arc_get_value(objects.obj32);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.obj32;
            lv_arc_set_value(objects.obj32, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_fan_3_rpm();
        const char *cur_val = lv_label_get_text(objects.obj33);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj33;
            lv_label_set_text(objects.obj33, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_fan_3_label();
        const char *cur_val = lv_label_get_text(objects.obj34);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj34;
            lv_label_set_text(objects.obj34, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_host_meta();
        const char *cur_val = lv_label_get_text(objects.fan_host_meta);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.fan_host_meta;
            lv_label_set_text(objects.fan_host_meta, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_fan_1_label();
        const char *cur_val = lv_label_get_text(objects.obj38);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj38;
            lv_label_set_text(objects.obj38, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_fan_1_rpm();
        const char *cur_val = lv_label_get_text(objects.obj39);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj39;
            lv_label_set_text(objects.obj39, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_fan_1_label();
        const char *cur_val = lv_label_get_text(objects.obj40);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj40;
            lv_label_set_text(objects.obj40, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_fan_1_rpm();
        const char *cur_val = lv_label_get_text(objects.obj41);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj41;
            lv_label_set_text(objects.obj41, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_fan_1_label();
        const char *cur_val = lv_label_get_text(objects.obj42);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj42;
            lv_label_set_text(objects.obj42, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_fan_1_rpm();
        const char *cur_val = lv_label_get_text(objects.obj43);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj43;
            lv_label_set_text(objects.obj43, new_val);
            tick_value_change_obj = NULL;
        }
    }
}

void create_screen_meteo() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.meteo = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_add_event_cb(obj, action_ui_swipe, LV_EVENT_GESTURE, (void *)0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(obj, &ui_font_ui_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // ui_meteo_clock
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.ui_meteo_clock = obj;
            lv_obj_set_pos(obj, 235, 15);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
            lv_obj_set_style_text_font(obj, &ui_font_test, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_EDITED);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_EDITED);
            lv_label_set_text(obj, "");
        }
        {
            // ui_meteo_img
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.ui_meteo_img = obj;
            lv_obj_set_pos(obj, 31, 16);
            lv_obj_set_size(obj, 150, 150);
            lv_img_set_src(obj, &img_clear_day);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_ADV_HITTEST|LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
        }
        {
            // ui_meteo_date
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.ui_meteo_date = obj;
            lv_obj_set_pos(obj, 235, 70);
            lv_obj_set_size(obj, 200, LV_SIZE_CONTENT);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
            lv_obj_set_style_text_font(obj, &ui_font_ui_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // ui_meteo_temp
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.ui_meteo_temp = obj;
            lv_obj_set_pos(obj, 255, 104);
            lv_obj_set_size(obj, 160, LV_SIZE_CONTENT);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
            lv_obj_set_style_text_font(obj, &ui_font_ui_40, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // ui_meteo_condition
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.ui_meteo_condition = obj;
            lv_obj_set_pos(obj, 185, 150);
            lv_obj_set_size(obj, 300, LV_SIZE_CONTENT);
            lv_label_set_long_mode(obj, LV_LABEL_LONG_DOT);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_line_create(parent_obj);
            objects.obj44 = obj;
            lv_obj_set_pos(obj, 25, 183);
            lv_obj_set_size(obj, 430, LV_SIZE_CONTENT);
            static lv_point_t line_points[] = {
                { 0, 0 },
                { 430, 0 }
            };
            lv_line_set_points(obj, line_points, 2);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
            lv_obj_set_style_line_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // ui_meteo_fi1
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.ui_meteo_fi1 = obj;
            lv_obj_set_pos(obj, 27, 205);
            lv_obj_set_size(obj, 50, 50);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_ADV_HITTEST|LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
        }
        {
            // ui_meteo_fi2
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.ui_meteo_fi2 = obj;
            lv_obj_set_pos(obj, 102, 205);
            lv_obj_set_size(obj, 50, 50);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_ADV_HITTEST|LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
        }
        {
            // ui_meteo_fi3
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.ui_meteo_fi3 = obj;
            lv_obj_set_pos(obj, 177, 205);
            lv_obj_set_size(obj, 50, 50);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_ADV_HITTEST|LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
        }
        {
            // ui_meteo_fi4
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.ui_meteo_fi4 = obj;
            lv_obj_set_pos(obj, 252, 205);
            lv_obj_set_size(obj, 50, 50);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_ADV_HITTEST|LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
        }
        {
            // ui_meteo_fi5
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.ui_meteo_fi5 = obj;
            lv_obj_set_pos(obj, 327, 205);
            lv_obj_set_size(obj, 50, 50);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_ADV_HITTEST|LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
        }
        {
            // ui_meteo_fi6
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.ui_meteo_fi6 = obj;
            lv_obj_set_pos(obj, 402, 205);
            lv_obj_set_size(obj, 50, 50);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_ADV_HITTEST|LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
        }
        {
            // ui_meteo_ft1_1
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.ui_meteo_ft1_1 = obj;
            lv_obj_set_pos(obj, 16, 256);
            lv_obj_set_size(obj, 72, LV_SIZE_CONTENT);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_line_create(parent_obj);
            objects.obj45 = obj;
            lv_obj_set_pos(obj, 24, 282);
            lv_obj_set_size(obj, 430, LV_SIZE_CONTENT);
            static lv_point_t line_points[] = {
                { 0, 0 },
                { 430, 0 }
            };
            lv_line_set_points(obj, line_points, 2);
            lv_obj_set_style_line_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // ui_meteo_fd1
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.ui_meteo_fd1 = obj;
            lv_obj_set_pos(obj, 40, 190);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
            lv_label_set_text(obj, "");
        }
        {
            // ui_meteo_fd2
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.ui_meteo_fd2 = obj;
            lv_obj_set_pos(obj, 115, 190);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
            lv_label_set_text(obj, "");
        }
        {
            // ui_meteo_fd3
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.ui_meteo_fd3 = obj;
            lv_obj_set_pos(obj, 190, 190);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
            lv_label_set_text(obj, "");
        }
        {
            // ui_meteo_fd4
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.ui_meteo_fd4 = obj;
            lv_obj_set_pos(obj, 265, 190);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
            lv_label_set_text(obj, "");
        }
        {
            // ui_meteo_fd5
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.ui_meteo_fd5 = obj;
            lv_obj_set_pos(obj, 340, 190);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
            lv_label_set_text(obj, "");
        }
        {
            // ui_meteo_fd6
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.ui_meteo_fd6 = obj;
            lv_obj_set_pos(obj, 415, 190);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_line_create(parent_obj);
            objects.obj46 = obj;
            lv_obj_set_pos(obj, 88, 188);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            static lv_point_t line_points[] = {
                { 0, 0 },
                { 0, 90 }
            };
            lv_line_set_points(obj, line_points, 2);
            lv_obj_set_style_line_color(obj, lv_color_hex(0x767676), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_line_opa(obj, 125, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_line_create(parent_obj);
            objects.obj47 = obj;
            lv_obj_set_pos(obj, 163, 188);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            static lv_point_t line_points[] = {
                { 0, 0 },
                { 0, 90 }
            };
            lv_line_set_points(obj, line_points, 2);
            lv_obj_set_style_line_color(obj, lv_color_hex(0x767676), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_line_opa(obj, 125, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_line_create(parent_obj);
            objects.obj48 = obj;
            lv_obj_set_pos(obj, 239, 188);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            static lv_point_t line_points[] = {
                { 0, 0 },
                { 0, 90 }
            };
            lv_line_set_points(obj, line_points, 2);
            lv_obj_set_style_line_color(obj, lv_color_hex(0x767676), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_line_opa(obj, 125, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_line_create(parent_obj);
            objects.obj49 = obj;
            lv_obj_set_pos(obj, 313, 188);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            static lv_point_t line_points[] = {
                { 0, 0 },
                { 0, 90 }
            };
            lv_line_set_points(obj, line_points, 2);
            lv_obj_set_style_line_color(obj, lv_color_hex(0x767676), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_line_opa(obj, 125, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_line_create(parent_obj);
            objects.obj50 = obj;
            lv_obj_set_pos(obj, 388, 188);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            static lv_point_t line_points[] = {
                { 0, 0 },
                { 0, 90 }
            };
            lv_line_set_points(obj, line_points, 2);
            lv_obj_set_style_line_color(obj, lv_color_hex(0x767676), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_line_opa(obj, 125, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // ui_meteo_ft1_2
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.ui_meteo_ft1_2 = obj;
            lv_obj_set_pos(obj, 91, 256);
            lv_obj_set_size(obj, 72, LV_SIZE_CONTENT);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // ui_meteo_ft1_3
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.ui_meteo_ft1_3 = obj;
            lv_obj_set_pos(obj, 166, 256);
            lv_obj_set_size(obj, 72, LV_SIZE_CONTENT);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // ui_meteo_ft1_4
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.ui_meteo_ft1_4 = obj;
            lv_obj_set_pos(obj, 241, 256);
            lv_obj_set_size(obj, 72, LV_SIZE_CONTENT);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // ui_meteo_ft1_5
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.ui_meteo_ft1_5 = obj;
            lv_obj_set_pos(obj, 316, 256);
            lv_obj_set_size(obj, 72, LV_SIZE_CONTENT);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // ui_meteo_ft1_6
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.ui_meteo_ft1_6 = obj;
            lv_obj_set_pos(obj, 391, 256);
            lv_obj_set_size(obj, 72, LV_SIZE_CONTENT);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_line_create(parent_obj);
            objects.obj51 = obj;
            lv_obj_set_pos(obj, 24, 309);
            lv_obj_set_size(obj, 430, LV_SIZE_CONTENT);
            static lv_point_t line_points[] = {
                { 0, 0 },
                { 430, 0 }
            };
            lv_line_set_points(obj, line_points, 2);
            lv_obj_set_style_line_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // aler_meteo
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.aler_meteo = obj;
            lv_obj_set_pos(obj, 24, 283);
            lv_obj_set_size(obj, 431, 27);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj52 = obj;
                    lv_obj_set_pos(obj, 168, 5);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &ui_font_ui_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
            }
        }
    }
    
    tick_screen_meteo();
}

void tick_screen_meteo() {
    {
        const char *new_val = get_var_ui_meteo_houre();
        const char *cur_val = lv_label_get_text(objects.ui_meteo_clock);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.ui_meteo_clock;
            lv_label_set_text(objects.ui_meteo_clock, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ui_meteo_date();
        const char *cur_val = lv_label_get_text(objects.ui_meteo_date);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.ui_meteo_date;
            lv_label_set_text(objects.ui_meteo_date, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ui_meteo_temp();
        const char *cur_val = lv_label_get_text(objects.ui_meteo_temp);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.ui_meteo_temp;
            lv_label_set_text(objects.ui_meteo_temp, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ui_meteo_condition();
        const char *cur_val = lv_label_get_text(objects.ui_meteo_condition);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.ui_meteo_condition;
            lv_label_set_text(objects.ui_meteo_condition, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ui_meteo_ft1();
        const char *cur_val = lv_label_get_text(objects.ui_meteo_ft1_1);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.ui_meteo_ft1_1;
            lv_label_set_text(objects.ui_meteo_ft1_1, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ui_meteo_fd1();
        const char *cur_val = lv_label_get_text(objects.ui_meteo_fd1);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.ui_meteo_fd1;
            lv_label_set_text(objects.ui_meteo_fd1, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ui_meteo_fd2();
        const char *cur_val = lv_label_get_text(objects.ui_meteo_fd2);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.ui_meteo_fd2;
            lv_label_set_text(objects.ui_meteo_fd2, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ui_meteo_fd3();
        const char *cur_val = lv_label_get_text(objects.ui_meteo_fd3);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.ui_meteo_fd3;
            lv_label_set_text(objects.ui_meteo_fd3, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ui_meteo_fd4();
        const char *cur_val = lv_label_get_text(objects.ui_meteo_fd4);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.ui_meteo_fd4;
            lv_label_set_text(objects.ui_meteo_fd4, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ui_meteo_fd5();
        const char *cur_val = lv_label_get_text(objects.ui_meteo_fd5);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.ui_meteo_fd5;
            lv_label_set_text(objects.ui_meteo_fd5, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ui_meteo_fd6();
        const char *cur_val = lv_label_get_text(objects.ui_meteo_fd6);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.ui_meteo_fd6;
            lv_label_set_text(objects.ui_meteo_fd6, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ui_meteo_ft2();
        const char *cur_val = lv_label_get_text(objects.ui_meteo_ft1_2);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.ui_meteo_ft1_2;
            lv_label_set_text(objects.ui_meteo_ft1_2, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ui_meteo_ft3();
        const char *cur_val = lv_label_get_text(objects.ui_meteo_ft1_3);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.ui_meteo_ft1_3;
            lv_label_set_text(objects.ui_meteo_ft1_3, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ui_meteo_ft4();
        const char *cur_val = lv_label_get_text(objects.ui_meteo_ft1_4);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.ui_meteo_ft1_4;
            lv_label_set_text(objects.ui_meteo_ft1_4, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ui_meteo_ft5();
        const char *cur_val = lv_label_get_text(objects.ui_meteo_ft1_5);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.ui_meteo_ft1_5;
            lv_label_set_text(objects.ui_meteo_ft1_5, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ui_meteo_ft6();
        const char *cur_val = lv_label_get_text(objects.ui_meteo_ft1_6);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.ui_meteo_ft1_6;
            lv_label_set_text(objects.ui_meteo_ft1_6, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ui_meteo_alert();
        const char *cur_val = lv_label_get_text(objects.obj52);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj52;
            lv_label_set_text(objects.obj52, new_val);
            tick_value_change_obj = NULL;
        }
    }
}

void create_screen_start() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.start = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // ui_start_bar
            lv_obj_t *obj = lv_bar_create(parent_obj);
            objects.ui_start_bar = obj;
            lv_obj_set_pos(obj, 165, 155);
            lv_obj_set_size(obj, 150, 10);
        }
        {
            // ui_start_bar_texte
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.ui_start_bar_texte = obj;
            lv_obj_set_pos(obj, 140, 127);
            lv_obj_set_size(obj, 200, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_ui_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj53 = obj;
            lv_obj_set_pos(obj, 160, 65);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_ui_40, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, _("PluseMon"));
        }
    }
    
    tick_screen_start();
}

void tick_screen_start() {
    {
        int32_t new_val = get_var_ui_start_bar();
        int32_t cur_val = lv_bar_get_value(objects.ui_start_bar);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.ui_start_bar;
            lv_bar_set_value(objects.ui_start_bar, new_val, LV_ANIM_ON);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ui_start_bar_texte();
        const char *cur_val = lv_label_get_text(objects.ui_start_bar_texte);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.ui_start_bar_texte;
            lv_label_set_text(objects.ui_start_bar_texte, new_val);
            tick_value_change_obj = NULL;
        }
    }
}

typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_main,
    tick_screen_gpu,
    tick_screen_fan,
    tick_screen_meteo,
    tick_screen_start,
};
void tick_screen(int screen_index) {
    if (screen_index >= 0 && screen_index < 5) {
        tick_screen_funcs[screen_index]();
    }
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen(screenId - 1);
}

//
// Fonts
//

ext_font_desc_t fonts[] = {
    { "font_18", &ui_font_font_18 },
    { "test", &ui_font_test },
    { "ui_16", &ui_font_ui_16 },
    { "ui_18", &ui_font_ui_18 },
    { "ui_40", &ui_font_ui_40 },
#if LV_FONT_MONTSERRAT_8
    { "MONTSERRAT_8", &lv_font_montserrat_8 },
#endif
#if LV_FONT_MONTSERRAT_10
    { "MONTSERRAT_10", &lv_font_montserrat_10 },
#endif
#if LV_FONT_MONTSERRAT_12
    { "MONTSERRAT_12", &lv_font_montserrat_12 },
#endif
#if LV_FONT_MONTSERRAT_14
    { "MONTSERRAT_14", &lv_font_montserrat_14 },
#endif
#if LV_FONT_MONTSERRAT_16
    { "MONTSERRAT_16", &lv_font_montserrat_16 },
#endif
#if LV_FONT_MONTSERRAT_18
    { "MONTSERRAT_18", &lv_font_montserrat_18 },
#endif
#if LV_FONT_MONTSERRAT_20
    { "MONTSERRAT_20", &lv_font_montserrat_20 },
#endif
#if LV_FONT_MONTSERRAT_22
    { "MONTSERRAT_22", &lv_font_montserrat_22 },
#endif
#if LV_FONT_MONTSERRAT_24
    { "MONTSERRAT_24", &lv_font_montserrat_24 },
#endif
#if LV_FONT_MONTSERRAT_26
    { "MONTSERRAT_26", &lv_font_montserrat_26 },
#endif
#if LV_FONT_MONTSERRAT_28
    { "MONTSERRAT_28", &lv_font_montserrat_28 },
#endif
#if LV_FONT_MONTSERRAT_30
    { "MONTSERRAT_30", &lv_font_montserrat_30 },
#endif
#if LV_FONT_MONTSERRAT_32
    { "MONTSERRAT_32", &lv_font_montserrat_32 },
#endif
#if LV_FONT_MONTSERRAT_34
    { "MONTSERRAT_34", &lv_font_montserrat_34 },
#endif
#if LV_FONT_MONTSERRAT_36
    { "MONTSERRAT_36", &lv_font_montserrat_36 },
#endif
#if LV_FONT_MONTSERRAT_38
    { "MONTSERRAT_38", &lv_font_montserrat_38 },
#endif
#if LV_FONT_MONTSERRAT_40
    { "MONTSERRAT_40", &lv_font_montserrat_40 },
#endif
#if LV_FONT_MONTSERRAT_42
    { "MONTSERRAT_42", &lv_font_montserrat_42 },
#endif
#if LV_FONT_MONTSERRAT_44
    { "MONTSERRAT_44", &lv_font_montserrat_44 },
#endif
#if LV_FONT_MONTSERRAT_46
    { "MONTSERRAT_46", &lv_font_montserrat_46 },
#endif
#if LV_FONT_MONTSERRAT_48
    { "MONTSERRAT_48", &lv_font_montserrat_48 },
#endif
};

//
// Color themes
//

uint32_t active_theme_index = 0;

//
//
//

void create_screens() {

// Set default LVGL theme
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), false, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    
    // Initialize screens
    // Create screens
    create_screen_main();
    create_screen_gpu();
    create_screen_fan();
    create_screen_meteo();
    create_screen_start();
}