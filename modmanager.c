#include "libra/mod.h"
#include "libra/platform.h"
#include "libra/table_of_contents.h"
#include "gui/common.h"
#include "gui/settings.h"

static GLFWwindow* window;
static int window_width;
static int window_height;

static char bin_dir[RA_MAX_PATH];
static char settings_path[RA_MAX_PATH];
static Settings settings;
static RA_Mod* mods;
static u32 mod_count;
static char filter[1024];

static void update(f32 frame_time);
static void draw_gui();
static void mod_list();
static void refresh();
static void install_mods();
static void no_game_folder_message();

int main(int argc, char** argv) {
	RA_Result result;
	
	window = GUI_startup("Ripped Apart Mod Manager", 960, 600);
	
	RA_remove_file_name(bin_dir, RA_MAX_PATH, argv[0]);
	if(snprintf(settings_path, sizeof(settings_path), "%s/settings.json", bin_dir) < 0) {
		RA_message_box(GUI_MESSAGE_BOX_ERROR, "Error", "Settings file path too long.");
	}
	
	if(RA_file_exists(settings_path)) {
		if((result = GUI_settings_read(&settings, settings_path)) != RA_SUCCESS) {
			RA_message_box(GUI_MESSAGE_BOX_ERROR, "Error", "Failed to read settings file (%s).", result->message);
		}
	}
	
	if(settings.game_dir_valid) {
		if((result = RA_mod_list_load(&mods, &mod_count, settings.game_dir)) != RA_SUCCESS) {
			RA_message_box(GUI_MESSAGE_BOX_ERROR, "Error", result->message);
		}
	} else {
		no_game_folder_message();
	}
	
	GUI_main_loop(window, update);
	GUI_shutdown(window);
}

static void update(f32 frame_time) {
	draw_gui();
	igRender();
	glfwMakeContextCurrent(window);
	glfwGetFramebufferSize(window, &window_width, &window_height);
}

extern const char* git_commit;
extern const char* git_tag;

static void draw_gui() {
	RA_Result result;
	
	f32 button_height = igGetFont()->FontSize + igGetStyle()->FramePadding.y * 2.f;
	f32 buttons_window_height = button_height + igGetStyle()->WindowPadding.y * 2.f;
	
	ImVec2 main_pos = {0, 0};
	ImVec2 main_size = {window_width, window_height - buttons_window_height};
	igSetNextWindowPos(main_pos, ImGuiCond_Always, (ImVec2) {0, 0});
	igSetNextWindowSize(main_size, ImGuiCond_Always);
	igBegin("main", NULL, ImGuiWindowFlags_NoDecoration);
	
	mod_list();
	
	igEnd();
	
	ImVec2 buttons_pos = {0, window_height - buttons_window_height};
	ImVec2 buttons_size = {window_width, buttons_window_height};
	igSetNextWindowPos(buttons_pos, ImGuiCond_Always, (ImVec2) {0, 0});
	igSetNextWindowSize(buttons_size, ImGuiCond_Always);
	igBegin("buttons", NULL, ImGuiWindowFlags_NoDecoration);
	
	if(igButton("Settings", (ImVec2) {0, 0})) {
		GUI_settings_open(&settings);
	}
	
	igSameLine(0.f, -1.f);
	if(igButton("Open Mods Folder", (ImVec2) {0, 0})) {
		if(settings.game_dir_valid) {
			char mods_dir[RA_MAX_PATH];
			if(snprintf(mods_dir, RA_MAX_PATH, "%s/mods", settings.game_dir) >= 0) {
				RA_open_file_path_or_url(mods_dir);
			}
		} else {
			no_game_folder_message();
		}
	}
	
	igSameLine(0.f, -1.f);
	if(igButton("Refresh", (ImVec2) {0, 0})) {
		if(settings.game_dir_valid) {
			refresh();
		} else {
			no_game_folder_message();
		}
	}
	
	igSameLine(0.f, -1.f);
	if(igButton("···", (ImVec2) {0, 0})) {
		igOpenPopup_Str("more_buttons", ImGuiPopupFlags_None);
	}
	
	
	if(git_tag && strlen(git_tag) > 0) {
		igSameLine(0.f, -1.f);
		igText("%s", git_tag);
	} else if(git_commit && strlen(git_commit) > 0) {
		igSameLine(0.f, -1.f);
		igText("Built from commit %c%c%c%c%c%c%c",
			git_commit[0], git_commit[1], git_commit[2], git_commit[3],
			git_commit[4], git_commit[5], git_commit[6]);
	}
	
	ImVec2 install_mods_text_size;
	igCalcTextSize(&install_mods_text_size, "Install Mods", NULL, false, -1.f);
	ImGuiStyle* s = igGetStyle();
	f32 install_mods_button_width = s->FramePadding.x + install_mods_text_size.x + s->FramePadding.x;
	f32 install_mods_area_width = install_mods_button_width + s->WindowPadding.x;
	igSameLine(0.f, -1.f);
	igSetCursorPosX(window_width - install_mods_area_width);
	if(igButton("Install Mods", (ImVec2) {0, 0})) {
		if(settings.game_dir_valid) {
			install_mods();
		} else {
			no_game_folder_message();
		}
	}
	
	if(igBeginPopup("more_buttons", ImGuiWindowFlags_None)) {
		if(igSelectable_Bool("GitHub", false, ImGuiSelectableFlags_None, (ImVec2) {0, 0})) {
			RA_open_file_path_or_url("https://github.com/chaoticgd/ripped_apart");
		}
		if(igSelectable_Bool("Nexus Mods", false, ImGuiSelectableFlags_None, (ImVec2) {0, 0})) {
			RA_open_file_path_or_url("https://www.nexusmods.com/ratchetandclankriftapart/mods/26");
		}
		igSameLine(0.f, -1.f);
		igEndPopup();
	}
	
	if(GUI_settings_draw(&settings, window_width, window_height)) {
		if((result = GUI_settings_write(&settings, settings_path)) != RA_SUCCESS) {
			RA_message_box(GUI_MESSAGE_BOX_ERROR, "Error", "Failed to write settings file (%s).", result->message);
			char mods_dir[RA_MAX_PATH];
			if(snprintf(mods_dir, RA_MAX_PATH, "%s/mods", settings.game_dir) < 0) {
				RA_message_box(GUI_MESSAGE_BOX_ERROR, "Error", "Path for 'mods' folder too long.");
			}
			RA_make_dir(mods_dir);
			char modcache_dir[RA_MAX_PATH];
			if(snprintf(modcache_dir, RA_MAX_PATH, "%s/modcache", settings.game_dir) < 0) {
				RA_message_box(GUI_MESSAGE_BOX_ERROR, "Error", "Path for 'modcache' folder too long.");
			}
			RA_make_dir(modcache_dir);
		}
		refresh();
	}
	
	igEnd();
}

static void mod_list() {
	igSetNextItemWidth(-1.f);
	igInputTextWithHint("##filter", "Filter", filter, sizeof(filter), ImGuiInputTextFlags_None, NULL, NULL);
	
	igBeginChild_Str("mod_list", (ImVec2) {0, 0}, false, ImGuiWindowFlags_None);
	
	ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable;
	if(igBeginTable("mod_table", 2, flags, (ImVec2) {0.f, 0.f}, 0.f)) {
		for(u32 i = 0; i < mod_count; i++) {
			RA_Mod* mod = &mods[i];
			
			if(RA_string_find_substring_no_case(mod->file_name, filter)) {
				igPushID_Int(i);
				igTableNextRow(ImGuiTableFlags_None, 0.f);
				igTableNextColumn();
				if(igSelectable_Bool(mod->file_name, mod->enabled, ImGuiSelectableFlags_SpanAllColumns, (ImVec2) {0, 0})) {
					mod->enabled = !mod->enabled;
				}
				igTableNextColumn();
				if(mod->author != NULL) {
					igText("%s", mod->author);
				}
				igPopID();
			}
		}
		
		igEndTable();
	}
	igPopStyleVar(2);
	
	igEndChild();
}

static void refresh() {
	RA_Result result;
	
	RA_mod_list_free(mods, mod_count);
	if((result = RA_mod_list_load(&mods, &mod_count, settings.game_dir))) {
		RA_message_box(GUI_MESSAGE_BOX_ERROR, "Error", "Failed to load mod (%s).", result->message);
	}
}

static void install_mods() {
	RA_Result result;
	
	char toc_path[RA_MAX_PATH];
	if(snprintf(toc_path, RA_MAX_PATH, "%s/toc", settings.game_dir) < 0) {
		RA_message_box(GUI_MESSAGE_BOX_ERROR, "Error", "TOC path too long.\n");
		return;
	}
	char toc_backup_path[RA_MAX_PATH];
	if(snprintf(toc_backup_path, RA_MAX_PATH, "%s/toc.bak", settings.game_dir) < 0) {
		RA_message_box(GUI_MESSAGE_BOX_ERROR, "Error", "TOC backup path too long.\n");
		return;
	}
	
	if(!RA_file_exists(toc_backup_path)) {
		u8* backup_data;
		s64 backup_size;
		if((result = RA_file_read(toc_path, &backup_data, &backup_size)) != RA_SUCCESS) {
			RA_message_box(GUI_MESSAGE_BOX_ERROR, "Error", "Failed to read toc file (%s).\n", result->message);
			return;
		}
		
		if((result = RA_file_write(toc_backup_path, backup_data, backup_size)) != RA_SUCCESS) {
			RA_message_box(GUI_MESSAGE_BOX_ERROR, "Error", "Failed to write backup toc file (%s). The table of contents has not been modified.\n", result->message);
			free(backup_data);
			return;
		}
		
		free(backup_data);
	}
	
	u8* in_data;
	s64 in_size;
	if((result = RA_file_read(toc_backup_path, &in_data, &in_size)) != RA_SUCCESS) {
		RA_message_box(GUI_MESSAGE_BOX_ERROR, "Error", "Failed to read toc file (%s).\n", result->message);
		return;
	}
	
	RA_TableOfContents toc;
	if((RA_toc_parse(&toc, in_data, (u32) in_size)) != RA_SUCCESS) {
		RA_message_box(GUI_MESSAGE_BOX_ERROR, "Error", "Failed to parse toc file (%s). "
			"Try restoring to the toc.bak file, or delete the toc file and validate files in Steam.\n",
			result->message);
		free(in_data);
		return;
	}
	
	u32 enabled_mod_count;
	if((result = RA_mod_list_rebuild_toc(mods, mod_count, &toc, &enabled_mod_count)) != RA_SUCCESS) {
		RA_message_box(GUI_MESSAGE_BOX_ERROR, "Error", "Failed to install mods (%s). The table of contents has not been modified.\n", result->message);
		RA_toc_free(&toc, FREE_FILE_DATA);
		return;
	}
	
	u8* out_data;
	s64 out_size;
	if((result = RA_toc_build(&toc, &out_data, &out_size)) != RA_SUCCESS) {
		RA_message_box(GUI_MESSAGE_BOX_ERROR, "Error", "Failed to build toc (%s). The table of contents has not been modified.\n", result->message);
		RA_toc_free(&toc, FREE_FILE_DATA);
		return;
	}
	
	if((result = RA_file_write(toc_path, out_data, out_size)) != RA_SUCCESS) {
		RA_message_box(GUI_MESSAGE_BOX_ERROR, "Error", "Failed to write toc file (%s).\n", result->message);
		free(out_data);
		RA_toc_free(&toc, FREE_FILE_DATA);
		return;
	}
	
	RA_message_box(GUI_MESSAGE_BOX_INFO, "Success", "Installed %u mod%s successfully.", enabled_mod_count, enabled_mod_count == 1 ? "" : "s");
	
	free(out_data);
	RA_toc_free(&toc, FREE_FILE_DATA);
}

static void no_game_folder_message() {
	RA_message_box(GUI_MESSAGE_BOX_INFO, "No Game Folder", "No valid game folder set. Click on 'Settings' to specify one.");
}
