#include "menu.h"

static const ImVec2 zero = {0, 0};
static const ImVec2 tab_button_size = {64, 64};
static const f32 margin = 32.f;

static char selected_tab[64] = "Home";

void GUI_menu_begin(f32 menu_width, f32 menu_height) {
	ImVec2 body_pos = {margin + tab_button_size.x, margin};
	igSetNextWindowPos(body_pos, ImGuiCond_Always, zero);
	ImVec2 body_size = {menu_width - margin * 2, menu_height - margin * 2};
	igSetNextWindowSize(body_size, ImGuiCond_Always);
	
	igBegin("menu", NULL, ImGuiWindowFlags_NoDecoration);
	
	ImVec2 tabs_pos = {margin, margin};
	igSetNextWindowPos(tabs_pos, ImGuiCond_Always, zero);
	ImVec2 tabs_size = {tab_button_size.x, menu_height - margin * 2};
	igSetNextWindowSize(tabs_size, ImGuiCond_Always);
	
	igBegin("tabs", NULL, ImGuiWindowFlags_NoDecoration);
	igEnd();
}

b8 GUI_menu_tab(const char* title) {
	igBegin("tabs", NULL, ImGuiWindowFlags_NoDecoration);
	ImVec2 uv0 = {0, 0};
	ImVec2 uv1 = {1, 1};
	ImVec4 bg_col = {0, 0, 0, 0};
	igPushStyleVar_Vec2(ImGuiStyleVar_FramePadding, uv0);
	if(igImageButton(title, NULL, tab_button_size, uv0, uv1, bg_col, bg_col)) {
		RA_string_copy(selected_tab, title, sizeof(selected_tab) - 1);
	}
	igPopStyleVar(1);
	igEnd();
	
	return strcmp(title, selected_tab) == 0;
}

void GUI_menu_end() {
	igEnd();
}
