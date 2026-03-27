#ifndef UI_BACKEND_H
#define UI_BACKEND_H

#include <lvgl.h>

#if defined(UI_BACKEND_EEZ)
#include "ui/ui.h"
#include "ui/screens.h"

static inline lv_obj_t *ui_time_label(void)
{
    return objects.ui_meteo_clock;
}
#elif defined(UI_BACKEND_SQUARELINE)
#include "ui2/ui.h"
#include "ui2/screens/ui_Meteo.h"

static inline lv_obj_t *ui_time_label(void)
{
    return ui_clock;
}
#else
#error "Define UI_BACKEND_EEZ or UI_BACKEND_SQUARELINE to select the UI backend."
#endif

#endif
