#ifndef GUI_SETTINGS_H
#define GUI_SETTINGS_H

#include "common.h"

typedef struct {
	b8 game_dir_valid;
	char game_dir[RA_MAX_PATH];
} Settings;

extern Settings settings;

void GUI_settings_open();
b8 GUI_settings_draw(f32 window_width, f32 window_height);
void GUI_settings_read(Settings* settings, const char* path);
void GUI_settings_write(Settings* settings, const char* path);

#endif
