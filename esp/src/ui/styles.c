#include "styles.h"
#include "images.h"
#include "fonts.h"

#include "ui.h"
#include "screens.h"

//
// Style: defaut
//

void init_style_defaut_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_bg_color(style, lv_color_hex(0x0f1115));
    lv_style_set_text_font(style, &ui_font_ui_16);
};

lv_style_t *get_style_defaut_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_mem_alloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_defaut_MAIN_DEFAULT(style);
    }
    return style;
};

void add_style_defaut(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_defaut_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

void remove_style_defaut(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_defaut_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

//
// Style: defaut1
//

void init_style_defaut1_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_text_color(style, lv_color_hex(0xd8deea));
    lv_style_set_text_align(style, LV_TEXT_ALIGN_RIGHT);
    lv_style_set_text_font(style, &ui_font_ui_16);
};

lv_style_t *get_style_defaut1_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_mem_alloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_defaut1_MAIN_DEFAULT(style);
    }
    return style;
};

void add_style_defaut1(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_defaut1_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

void remove_style_defaut1(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_defaut1_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

//
// Style: titre
//

void init_style_titre_MAIN_DEFAULT(lv_style_t *style) {
    init_style_defaut1_MAIN_DEFAULT(style);
    
    lv_style_set_text_color(style, lv_color_hex(0x90a0bc));
    lv_style_set_border_side(style, LV_BORDER_SIDE_BOTTOM);
    lv_style_set_border_width(style, 1);
    lv_style_set_border_color(style, lv_color_hex(0xd31b1b));
};

lv_style_t *get_style_titre_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_mem_alloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_titre_MAIN_DEFAULT(style);
    }
    return style;
};

void add_style_titre(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_titre_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

void remove_style_titre(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_titre_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

//
//
//

void add_style(lv_obj_t *obj, int32_t styleIndex) {
    typedef void (*AddStyleFunc)(lv_obj_t *obj);
    static const AddStyleFunc add_style_funcs[] = {
        add_style_defaut,
        add_style_defaut1,
        add_style_titre,
    };
    add_style_funcs[styleIndex](obj);
}

void remove_style(lv_obj_t *obj, int32_t styleIndex) {
    typedef void (*RemoveStyleFunc)(lv_obj_t *obj);
    static const RemoveStyleFunc remove_style_funcs[] = {
        remove_style_defaut,
        remove_style_defaut1,
        remove_style_titre,
    };
    remove_style_funcs[styleIndex](obj);
}