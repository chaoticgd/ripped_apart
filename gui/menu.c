#include "menu.h"

static const ImVec2 zero = {0, 0};
static const ImVec2 tab_button_size = {64, 64};
static const f32 margin = 32.f;

static char selected_tab[64] = "Home";

void GUI_menu_begin(f32 menu_width, f32 menu_height) {
	ImVec2 body_pos = {margin + tab_button_size.x, margin};
	igSetNextWindowPos(body_pos, ImGuiCond_Always, zero);
	ImVec2 body_size = {menu_width - margin * 2 - tab_button_size.x, menu_height - margin * 2};
	igSetNextWindowSize(body_size, ImGuiCond_Always);
	
	igBegin("menu", NULL, ImGuiWindowFlags_NoDecoration);
	
	ImVec2 tabs_pos = {margin, margin};
	igSetNextWindowPos(tabs_pos, ImGuiCond_Always, zero);
	ImVec2 tabs_size = {tab_button_size.x, menu_height - margin * 2};
	igSetNextWindowSize(tabs_size, ImGuiCond_Always);
	
	igPushStyleColor_Vec4(ImGuiCol_WindowBg, igGetStyle()->Colors[ImGuiCol_Border]);
	igBegin("tabs", NULL, ImGuiWindowFlags_NoDecoration);
	igEnd();
	igPopStyleColor(1);
}

static b8 tab_button(const char* text, ImTextureID texture_id, bool selected) {
	ImGuiID id = igGetID_Str(text);
	
	ImGuiWindow* window = igGetCurrentWindow();
	if(window->SkipItems)
		return false;

	const ImRect bb = {window->DC.CursorPos, {window->DC.CursorPos.x + tab_button_size.x, window->DC.CursorPos.y + tab_button_size.y}};
	igItemSize_Rect(bb, 0.f);
	if(!igItemAdd(bb, id, NULL, ImGuiItemFlags_None))
		return false;

	bool hovered, held;
	bool pressed = igButtonBehavior(bb, id, &hovered, &held, ImGuiButtonFlags_None);

	// Render
	ImVec2 uv0 = {0, 0};
	ImVec2 uv1 = {1, 1};
	ImU32 bg_col = selected ? igGetColorU32_Col(ImGuiCol_WindowBg, 1.f) : 0;
	ImVec4 tint_col = {0, 0, 0, 0.f};
	const ImU32 col = igGetColorU32_Col((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button, 0.f);
	igRenderNavHighlight(bb, id, ImGuiNavHighlightFlags_None);
	igRenderFrame(bb.Min, bb.Max, col, true, igGetStyle()->FrameRounding);
	ImDrawList_AddRectFilled(window->DrawList, bb.Min, bb.Max, bg_col, 4.f, ImDrawFlags_None);
	ImDrawList_AddImage(window->DrawList, texture_id, bb.Min, bb.Max, uv0, uv1, igGetColorU32_Vec4(tint_col));
	ImDrawList_AddText_Vec2(window->DrawList, bb.Min, 0xffffffff, text, NULL);
	
	return pressed;
}

b8 GUI_menu_tab(const char* title) {
	igBegin("tabs", NULL, ImGuiWindowFlags_NoDecoration);
	bool selected = strcmp(title, selected_tab) == 0;
	if(tab_button(title, NULL, selected)) {
		RA_string_copy(selected_tab, title, sizeof(selected_tab) - 1);
		selected = true;
	}
	igEnd();
	return selected;
}

void GUI_menu_end() {
	igEnd();
}
