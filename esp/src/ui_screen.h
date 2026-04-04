#ifndef UI_SCREEN_H
#define UI_SCREEN_H

#include <stdbool.h>

#include "ui/screens.h"

#ifdef __cplusplus
extern "C" {
#endif

void ui_screen_start(void);
void ui_screen_set_active(enum ScreensEnum screen_id);
enum ScreensEnum ui_screen_get_active(void);
void ui_screen_set_fans_visibility(bool fan2_visible, bool fan3_visible);

#ifdef __cplusplus
}
#endif

#endif
