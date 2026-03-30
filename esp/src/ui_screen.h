#ifndef UI_SCREEN_H
#define UI_SCREEN_H

#include "ui/screens.h"

#ifdef __cplusplus
extern "C" {
#endif

void ui_screen_start(void);
void ui_screen_set_active(enum ScreensEnum screen_id);

#ifdef __cplusplus
}
#endif

#endif
