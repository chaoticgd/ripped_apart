#include "settings.h"

#include "../libra/platform.h"

Settings settings;
static Settings settings_scratch;

void GUI_settings_open() {
	igOpenPopup_Str("Settings", ImGuiPopupFlags_None);
	settings_scratch = settings;
}

b8 GUI_settings_draw(f32 window_width, f32 window_height) {
	b8 settings_modified = false;
	
	ImVec2 zero = {0, 0};
	ImVec2 size = {640.f, 200.f};
	ImVec2 pos = {(window_width - size.x) / 2.f, (window_height - size.y) / 2.f};
	igSetNextWindowPos(pos, ImGuiCond_Always, zero);
	igSetNextWindowSize(size, ImGuiCond_Always);
	igSetNextWindowFocus();
	if(igBeginPopupModal("Settings", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
		if(igInputText("Game Folder", settings_scratch.game_dir, sizeof(settings_scratch.game_dir), ImGuiInputTextFlags_None, NULL, NULL)) {
			char exe_path[RA_MAX_PATH];
			if(snprintf(exe_path, RA_MAX_PATH, "%s/RiftApart.exe", settings_scratch.game_dir) >= 0) {
				settings_scratch.game_dir_valid = RA_file_exists(exe_path);
			} else {
				settings_scratch.game_dir_valid = false;
			}
		}
		
		if(settings_scratch.game_dir_valid == false) {
			igPushStyleColor_U32(ImGuiCol_Text, 0xff0000ff);
			igText("RiftApart.exe not found!");
			igPopStyleColor(1);
		}
		
		igBeginDisabled(settings_scratch.game_dir_valid == false);
		
		f32 button_height = igGetFont()->FontSize + igGetStyle()->FramePadding.y * 2.f;
		igSetCursorPosY(size.y - (button_height + igGetStyle()->WindowPadding.y));
		if(igButton("Okay", zero)) {
			settings = settings_scratch;
			settings_modified = true;
			igCloseCurrentPopup();
		}
		
		igEndDisabled();
		
		igSameLine(0.f, -1.f);
		if(igButton("Cancel", zero)) {
			igCloseCurrentPopup();
		}
		
		igEnd();
	}
	
	return settings_modified;
}

void GUI_settings_read(Settings* settings, const char* path) {
	
}

void GUI_settings_write(Settings* settings, const char* path) {
	
}
