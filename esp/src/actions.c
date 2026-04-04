#include "ui/actions.h"

#include "ui_screen.h"
#include "ui/screens.h"

static lv_obj_t *screen_from_id(enum ScreensEnum target)
{
    switch (target) {
        case SCREEN_ID_MAIN:
            return objects.main;
        case SCREEN_ID_GPU:
            return objects.gpu;
        case SCREEN_ID_FAN:
            return objects.fan;
        default:
            return NULL;
    }
}

static void action_swipe_to(enum ScreensEnum target, lv_scr_load_anim_t anim)
{
    lv_obj_t *target_obj = screen_from_id(target);
    if (target_obj == NULL) {
        return;
    }
    lv_scr_load_anim(target_obj, anim, 220, 0, false);
    ui_screen_set_active(target);
    tick_screen_by_id(target);
}

void action_ui_swipe(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_GESTURE) {
        return;
    }

    lv_obj_t *target = lv_event_get_target(e);
    lv_indev_t *indev = lv_event_get_indev(e);
    if (indev == NULL) {
        indev = lv_indev_get_act();
    }
    if (indev == NULL) {
        return;
    }

    lv_dir_t dir = lv_indev_get_gesture_dir(indev);
    if (target == objects.main && dir == LV_DIR_LEFT) {
        action_swipe_to(SCREEN_ID_GPU, LV_SCR_LOAD_ANIM_MOVE_LEFT);
        lv_indev_wait_release(indev);
        return;
    }
    if (target == objects.gpu && dir == LV_DIR_RIGHT) {
        action_swipe_to(SCREEN_ID_MAIN, LV_SCR_LOAD_ANIM_MOVE_RIGHT);
        lv_indev_wait_release(indev);
        return;
    }
    if (target == objects.gpu && dir == LV_DIR_LEFT) {
        action_swipe_to(SCREEN_ID_FAN, LV_SCR_LOAD_ANIM_MOVE_LEFT);
        lv_indev_wait_release(indev);
        return;
    }
    if (target == objects.fan && dir == LV_DIR_RIGHT) {
        action_swipe_to(SCREEN_ID_GPU, LV_SCR_LOAD_ANIM_MOVE_RIGHT);
        lv_indev_wait_release(indev);
    }
}
