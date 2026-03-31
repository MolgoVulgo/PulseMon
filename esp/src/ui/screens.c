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

//
// Screens
//

void create_screen_main() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.main = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 340);
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
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff171a21), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff2a3140), LV_PART_MAIN | LV_STATE_DEFAULT);
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
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "CPU usage");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj1 = obj;
                    lv_obj_set_pos(obj, 0, 48);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Temp");
                }
                {
                    // cpu_temp
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.cpu_temp = obj;
                    lv_obj_set_pos(obj, 49, 48);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffd8deea), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
            }
        }
        {
            // memory
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.memory = obj;
            lv_obj_set_pos(obj, 163, 34);
            lv_obj_set_size(obj, 154, 97);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff171a21), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff2a3140), LV_PART_MAIN | LV_STATE_DEFAULT);
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
                    objects.obj2 = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Memory");
                }
                {
                    // mem_pct
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.mem_pct = obj;
                    lv_obj_set_pos(obj, 0, 16);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffd8deea), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj3 = obj;
                    lv_obj_set_pos(obj, 0, 40);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Used");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj4 = obj;
                    lv_obj_set_pos(obj, 0, 57);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Total");
                }
                {
                    // mem_used
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.mem_used = obj;
                    lv_obj_set_pos(obj, 51, 40);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffd8deea), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
                {
                    // mem_total
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.mem_total = obj;
                    lv_obj_set_pos(obj, 51, 57);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffd8deea), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
            }
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.obj5 = obj;
            lv_obj_set_pos(obj, 320, 34);
            lv_obj_set_size(obj, 154, 97);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff171a21), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff2a3140), LV_PART_MAIN | LV_STATE_DEFAULT);
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
                    objects.obj6 = obj;
                    lv_obj_set_pos(obj, 43, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "GPU usage");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj7 = obj;
                    lv_obj_set_pos(obj, -4, 40);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Temp");
                }
                {
                    // gpu_temp
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.gpu_temp = obj;
                    lv_obj_set_pos(obj, 60, 40);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffd8deea), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj8 = obj;
                    lv_obj_set_pos(obj, -4, 57);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Power");
                }
                {
                    // gpu_power
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.gpu_power = obj;
                    lv_obj_set_pos(obj, 52, 57);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffd8deea), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
                {
                    // gpu_pct
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.gpu_pct = obj;
                    lv_obj_set_pos(obj, -4, 16);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffd8deea), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
            }
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj9 = obj;
            lv_obj_set_pos(obj, 12, 8);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffd8deea), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_font_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "Stats Linux Monitor");
        }
        {
            // host_meta
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.host_meta = obj;
            lv_obj_set_pos(obj, 240, 12);
            lv_obj_set_size(obj, 228, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // cpu_pct
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.cpu_pct = obj;
            lv_obj_set_pos(obj, 15, 59);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffd8deea), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // usage_panel
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.usage_panel = obj;
            lv_obj_set_pos(obj, 6, 135);
            lv_obj_set_size(obj, 232, 160);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff0f131d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff232a38), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // temp_panel
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.temp_panel = obj;
            lv_obj_set_pos(obj, 242, 135);
            lv_obj_set_size(obj, 232, 160);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff0f131d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff232a38), LV_PART_MAIN | LV_STATE_DEFAULT);
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
    lv_obj_set_size(obj, 480, 340);
    lv_obj_add_event_cb(obj, action_ui_swipe, LV_EVENT_GESTURE, (void *)0);
    add_style_defaut(obj);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.obj10 = obj;
            lv_obj_set_pos(obj, 6, 47);
            lv_obj_set_size(obj, 153, 50);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff171a21), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff2a3140), LV_PART_MAIN | LV_STATE_DEFAULT);
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
                    objects.obj11 = obj;
                    lv_obj_set_pos(obj, -4, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "GPU Usage");
                }
                {
                    // gpu_pct_1
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.gpu_pct_1 = obj;
                    lv_obj_set_pos(obj, -4, 16);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffd8deea), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
            }
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj12 = obj;
            lv_obj_set_pos(obj, 12, 21);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffd8deea), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_font_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "Stats Linux Monitor [");
        }
        {
            // host_meta_1
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.host_meta_1 = obj;
            lv_obj_set_pos(obj, 240, 25);
            lv_obj_set_size(obj, 228, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // graph_gpu_pct
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.graph_gpu_pct = obj;
            lv_obj_set_pos(obj, 6, 160);
            lv_obj_set_size(obj, 232, 160);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff0f131d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff232a38), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.obj13 = obj;
            lv_obj_set_pos(obj, 163, 47);
            lv_obj_set_size(obj, 154, 50);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff171a21), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff2a3140), LV_PART_MAIN | LV_STATE_DEFAULT);
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
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Temperature");
                }
                {
                    // gpu_temp_1
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.gpu_temp_1 = obj;
                    lv_obj_set_pos(obj, -4, 16);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffd8deea), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
            }
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.obj15 = obj;
            lv_obj_set_pos(obj, 321, 47);
            lv_obj_set_size(obj, 153, 50);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff171a21), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff2a3140), LV_PART_MAIN | LV_STATE_DEFAULT);
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
                    objects.obj16 = obj;
                    lv_obj_set_pos(obj, -4, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Power");
                }
                {
                    // gpu_power_1
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.gpu_power_1 = obj;
                    lv_obj_set_pos(obj, -4, 16);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffd8deea), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
            }
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.obj17 = obj;
            lv_obj_set_pos(obj, 163, 101);
            lv_obj_set_size(obj, 153, 50);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff171a21), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff2a3140), LV_PART_MAIN | LV_STATE_DEFAULT);
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
                    objects.obj18 = obj;
                    lv_obj_set_pos(obj, -4, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Fan");
                }
                {
                    // gpu_pct_7
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.gpu_pct_7 = obj;
                    lv_obj_set_pos(obj, -4, 16);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffd8deea), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
            }
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj19 = obj;
            lv_obj_set_pos(obj, 202, 21);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffff6e39), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_font_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "GPU");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj20 = obj;
            lv_obj_set_pos(obj, 231, 21);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffd8deea), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_font_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "]");
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.obj21 = obj;
            lv_obj_set_pos(obj, 6, 100);
            lv_obj_set_size(obj, 153, 50);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff171a21), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff2a3140), LV_PART_MAIN | LV_STATE_DEFAULT);
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
                    objects.obj22 = obj;
                    lv_obj_set_pos(obj, -4, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff90a0bc), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "VRAM");
                }
                {
                    // gpu_vram_total
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.gpu_vram_total = obj;
                    lv_obj_set_pos(obj, 65, 0);
                    lv_obj_set_size(obj, 70, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffd8deea), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
                {
                    lv_obj_t *obj = lv_bar_create(parent_obj);
                    objects.obj23 = obj;
                    lv_obj_set_pos(obj, -4, 17);
                    lv_obj_set_size(obj, 139, 15);
                }
            }
        }
        {
            // graph_gpu_temp
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.graph_gpu_temp = obj;
            lv_obj_set_pos(obj, 242, 160);
            lv_obj_set_size(obj, 232, 160);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff0f131d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff232a38), LV_PART_MAIN | LV_STATE_DEFAULT);
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
        int32_t cur_val = lv_bar_get_value(objects.obj23);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.obj23;
            lv_bar_set_value(objects.obj23, new_val, LV_ANIM_OFF);
            tick_value_change_obj = NULL;
        }
    }
}

typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_main,
    tick_screen_gpu,
};
void tick_screen(int screen_index) {
    tick_screen_funcs[screen_index]();
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen_funcs[screenId - 1]();
}

//
// Fonts
//

ext_font_desc_t fonts[] = {
    { "font_18", &ui_font_font_18 },
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
}