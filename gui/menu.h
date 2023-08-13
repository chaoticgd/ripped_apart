#ifndef GUI_MENU_H
#define GUI_MENU_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

void GUI_menu_begin(f32 menu_width, f32 menu_height);
b8 GUI_menu_tab(const char* title);
void GUI_menu_end();

#ifdef __cplusplus
}
#endif

#endif
