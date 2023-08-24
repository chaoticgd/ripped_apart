#include "settings.h"

#include <json_object.h>
#include <json_tokener.h>

#include "../libra/platform.h"

static Settings settings_scratch;

static b8 is_valid_game_dir(const char* game_dir);

void GUI_settings_open(Settings* settings) {
	igOpenPopup_Str("Settings", ImGuiPopupFlags_None);
	settings_scratch = *settings;
}

b8 GUI_settings_draw(Settings* settings, f32 window_width, f32 window_height) {
	b8 settings_modified = false;
	
	ImVec2 zero = {0, 0};
	ImVec2 size = {640.f, 200.f};
	ImVec2 pos = {(window_width - size.x) / 2.f, (window_height - size.y) / 2.f};
	igSetNextWindowPos(pos, ImGuiCond_Always, zero);
	igSetNextWindowSize(size, ImGuiCond_Always);
	igSetNextWindowFocus();
	if(igBeginPopupModal("Settings", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
		if(igInputText("Game Folder", settings_scratch.game_dir, sizeof(settings_scratch.game_dir), ImGuiInputTextFlags_None, NULL, NULL)) {
			settings_scratch.game_dir_valid = is_valid_game_dir(settings_scratch.game_dir);
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
			*settings = settings_scratch;
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

RA_Result GUI_settings_read(Settings* settings, const char* path) {
	RA_Result result;
	
	u8* data;
	s64 size;
	if((result = RA_file_read(path, &data, &size)) != RA_SUCCESS) {
		return result;
	}
	
	json_object* root = json_tokener_parse((char*) data);
	if(root == NULL) {
		RA_free(data);
		return RA_FAILURE("failed to parse settings file");
	}
	RA_free(data);
	
	json_object* game_dir_json = json_object_object_get(root, "game_dir");
	const char* game_dir = json_object_get_string(game_dir_json);
	if(game_dir) {
		RA_string_copy(settings->game_dir, game_dir, sizeof(settings->game_dir));
		settings->game_dir_valid = is_valid_game_dir(settings->game_dir);
	}
	
	json_object_put(root);
	return RA_SUCCESS;
}

RA_Result GUI_settings_write(Settings* settings, const char* path) {
	RA_Result result;
	
	json_object* root = json_object_new_object();
	
	json_object* game_dir = json_object_new_string(settings->game_dir);
	json_object_object_add(root, "game_dir", game_dir);
	
	const char* string = json_object_to_json_string(root);
	if((result = RA_file_write(path, (u8*) string, strlen(string))) != RA_SUCCESS) {
		return result;
	}
	
	json_object_put(root);
	return RA_SUCCESS;
}

static b8 is_valid_game_dir(const char* game_dir) {
	char exe_path[RA_MAX_PATH];
	if(snprintf(exe_path, RA_MAX_PATH, "%s/RiftApart.exe", game_dir) >= 0) {
		return RA_file_exists(exe_path);
	} else {
		return false;
	}
}
