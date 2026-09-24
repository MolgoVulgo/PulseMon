#ifndef UI_SCREEN_H
#define UI_SCREEN_H

#include <stdint.h>

#include "ui/screens.h"

#ifdef __cplusplus
extern "C" {
#endif

void ui_screen_start(void);
void ui_screen_set_start_progress(int32_t pct, const char *text);
void ui_screen_show_main_and_release_start(void);
void ui_screen_note_transition_start(enum ScreensEnum screen_id);
void ui_screen_set_active(enum ScreensEnum screen_id);
void ui_screen_load(enum ScreensEnum screen_id, lv_scr_load_anim_t anim);
enum ScreensEnum ui_screen_get_active(void);

#ifdef __cplusplus
}
#endif

#endif
