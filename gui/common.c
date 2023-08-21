#include "common.h"

#include <float.h>

#include "../libra/platform.h"
#include "font.h"

static void setup_style(ImGuiStyle* dst);

GLFWwindow* GUI_startup(const char* window_title, s32 window_width, s32 window_height) {
	if(!glfwInit()) {
		fprintf(stderr, "error: Failed to load GLFW.\n");
		abort();
	}
	
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	
	GLFWwindow* window = glfwCreateWindow(window_width, window_height, window_title, NULL, NULL);
	if(window == NULL) {
		RA_message_box(GUI_MESSAGE_BOX_ERROR, window_title, "Failed to open GLFW window.");
		abort();
	}
	
	glfwMakeContextCurrent(window);
	
	glfwSwapInterval(1);
	
	if(gladLoadGL() == 0) {
		RA_message_box(GUI_MESSAGE_BOX_ERROR, window_title, "Failed to load OpenGL.");
		abort();
	}
	
	igCreateContext(NULL);
	
	ImGui_ImplGlfw_InitForOpenGL(window, true);
 	ImGui_ImplOpenGL3_Init("#version 330");
	
	setup_style(NULL);
	
	return window;
}

void GUI_main_loop(GLFWwindow* window, void (*update)(f32 frame_time)) {
	f32 prev_time = glfwGetTime();
	f32 frame_time = 0.f;
	while(!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		igNewFrame();
		
		update(frame_time);
		
		ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());
		glfwSwapBuffers(window);
		
		int window_focused = glfwGetWindowAttrib(window, GLFW_FOCUSED);
		int window_hovered = glfwGetWindowAttrib(window, GLFW_HOVERED);
		if(!(window_focused || window_hovered)) {
			while(glfwGetTime() < prev_time + 0.2f) {
				RA_thread_sleep_ms(200);
			}
		}
		
		f32 time = glfwGetTime();
		frame_time = prev_time - time;
		prev_time = time;
	}
}

void GUI_shutdown(GLFWwindow* window) {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	igDestroyContext(NULL);
	
	glfwDestroyWindow(window);
	glfwTerminate();
}

static void setup_style(ImGuiStyle* dst) {
	ImGuiStyle* style = dst ? dst : igGetStyle();
	ImVec4* colours = style->Colors;
		
	colours[ImGuiCol_Text]                   = (ImVec4) {1.00f, 1.00f, 1.00f, 1.00f};
	colours[ImGuiCol_TextDisabled]           = (ImVec4) {1.00f, 1.00f, 1.00f, 0.50f};
	colours[ImGuiCol_WindowBg]               = (ImVec4) {0.10f, 0.15f, 0.20f, 1.00f};
	colours[ImGuiCol_ChildBg]                = colours[ImGuiCol_WindowBg];
	colours[ImGuiCol_PopupBg]                = colours[ImGuiCol_WindowBg];
	colours[ImGuiCol_Border]                 = (ImVec4) {0.05f, 0.10f, 0.15f, 1.00f};
	colours[ImGuiCol_BorderShadow]           = (ImVec4) {0.00f, 0.00f, 0.00f, 1.00f};
	colours[ImGuiCol_FrameBg]                = (ImVec4) {0.20f, 0.25f, 0.35f, 1.00f};
	colours[ImGuiCol_FrameBgHovered]         = (ImVec4) {0.25f, 0.30f, 0.40f, 1.00f};
	colours[ImGuiCol_FrameBgActive]          = (ImVec4) {0.26f, 0.59f, 0.98f, 0.67f};
	colours[ImGuiCol_TitleBg]                = (ImVec4) {0.05f, 0.10f, 0.15f, 1.00f};
	colours[ImGuiCol_TitleBgActive]          = (ImVec4) {0.20f, 0.25f, 0.35f, 1.00f};
	colours[ImGuiCol_TitleBgCollapsed]       = (ImVec4) {0.00f, 0.00f, 0.00f, 0.51f};
	colours[ImGuiCol_MenuBarBg]              = (ImVec4) {0.14f, 0.14f, 0.14f, 1.00f};
	colours[ImGuiCol_ScrollbarBg]            = (ImVec4) {0.02f, 0.02f, 0.02f, 0.53f};
	colours[ImGuiCol_ScrollbarGrab]          = (ImVec4) {0.31f, 0.31f, 0.31f, 1.00f};
	colours[ImGuiCol_ScrollbarGrabHovered]   = (ImVec4) {0.41f, 0.41f, 0.41f, 1.00f};
	colours[ImGuiCol_ScrollbarGrabActive]    = (ImVec4) {0.51f, 0.51f, 0.51f, 1.00f};
	colours[ImGuiCol_CheckMark]              = (ImVec4) {0.26f, 0.59f, 0.98f, 1.00f};
	colours[ImGuiCol_SliderGrab]             = (ImVec4) {0.24f, 0.52f, 0.88f, 1.00f};
	colours[ImGuiCol_SliderGrabActive]       = (ImVec4) {0.26f, 0.59f, 0.98f, 1.00f};
	colours[ImGuiCol_Button]                 = (ImVec4) {0.20f, 0.25f, 0.35f, 1.00f};
	colours[ImGuiCol_ButtonHovered]          = (ImVec4) {0.25f, 0.30f, 0.40f, 1.00f};
	colours[ImGuiCol_ButtonActive]           = (ImVec4) {0.10f, 0.15f, 0.20f, 1.00f};
	colours[ImGuiCol_Header]                 = (ImVec4) {0.30f, 0.35f, 0.45f, 1.00f};
	colours[ImGuiCol_HeaderHovered]          = (ImVec4) {0.25f, 0.30f, 0.40f, 1.00f};
	colours[ImGuiCol_HeaderActive]           = colours[ImGuiCol_ButtonActive];
	colours[ImGuiCol_Separator]              = colours[ImGuiCol_Border];
	colours[ImGuiCol_SeparatorHovered]       = (ImVec4) {0.10f, 0.40f, 0.75f, 0.78f};
	colours[ImGuiCol_SeparatorActive]        = (ImVec4) {0.10f, 0.40f, 0.75f, 1.00f};
	colours[ImGuiCol_ResizeGrip]             = (ImVec4) {0.26f, 0.59f, 0.98f, 0.20f};
	colours[ImGuiCol_ResizeGripHovered]      = (ImVec4) {0.26f, 0.59f, 0.98f, 0.67f};
	colours[ImGuiCol_ResizeGripActive]       = (ImVec4) {0.26f, 0.59f, 0.98f, 0.95f};
	igImLerp_Vec4(&colours[ImGuiCol_Tab], colours[ImGuiCol_Header], colours[ImGuiCol_TitleBgActive], 0.80f);
	colours[ImGuiCol_TabHovered]             = colours[ImGuiCol_HeaderHovered];
	igImLerp_Vec4(&colours[ImGuiCol_TabActive], colours[ImGuiCol_HeaderActive], colours[ImGuiCol_TitleBgActive], 0.60f);
	igImLerp_Vec4(&colours[ImGuiCol_TabUnfocused], colours[ImGuiCol_Tab], colours[ImGuiCol_TitleBg], 0.80f);
	igImLerp_Vec4(&colours[ImGuiCol_TabUnfocusedActive], colours[ImGuiCol_TabActive], colours[ImGuiCol_TitleBg], 0.40f);
	colours[ImGuiCol_DockingPreview]         = colours[ImGuiCol_HeaderActive]; // * ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
	colours[ImGuiCol_DockingEmptyBg]         = (ImVec4) {0.20f, 0.20f, 0.20f, 1.00f};
	colours[ImGuiCol_PlotLines]              = (ImVec4) {0.61f, 0.61f, 0.61f, 1.00f};
	colours[ImGuiCol_PlotLinesHovered]       = (ImVec4) {1.00f, 0.43f, 0.35f, 1.00f};
	colours[ImGuiCol_PlotHistogram]          = (ImVec4) {0.90f, 0.70f, 0.00f, 1.00f};
	colours[ImGuiCol_PlotHistogramHovered]   = (ImVec4) {1.00f, 0.60f, 0.00f, 1.00f};
	colours[ImGuiCol_TableHeaderBg]          = (ImVec4) {0.19f, 0.19f, 0.20f, 1.00f};
	colours[ImGuiCol_TableBorderStrong]      = (ImVec4) {0.31f, 0.31f, 0.35f, 1.00f};   // Prefer using Alpha=1.0 here
	colours[ImGuiCol_TableBorderLight]       = (ImVec4) {0.23f, 0.23f, 0.25f, 1.00f};   // Prefer using Alpha=1.0 here
	colours[ImGuiCol_TableRowBg]             = (ImVec4) {0.00f, 0.00f, 0.00f, 0.00f};
	colours[ImGuiCol_TableRowBgAlt]          = (ImVec4) {0.90f, 0.90f, 1.00f, 0.05f};
	colours[ImGuiCol_TextSelectedBg]         = (ImVec4) {0.26f, 0.59f, 0.98f, 0.35f};
	colours[ImGuiCol_DragDropTarget]         = (ImVec4) {1.00f, 1.00f, 0.00f, 0.90f};
	colours[ImGuiCol_NavHighlight]           = (ImVec4) {0.26f, 0.59f, 0.98f, 1.00f};
	colours[ImGuiCol_NavWindowingHighlight]  = (ImVec4) {1.00f, 1.00f, 1.00f, 0.70f};
	colours[ImGuiCol_NavWindowingDimBg]      = (ImVec4) {0.80f, 0.80f, 0.80f, 0.20f};
	colours[ImGuiCol_ModalWindowDimBg]       = (ImVec4) {0.00f, 0.00f, 0.00f, 0.25f};
	
	style->WindowBorderSize = 4.f;
	
	ImFontConfig font_cfg;
	memset(&font_cfg, 0, sizeof(ImFontConfig));
	font_cfg.FontDataOwnedByAtlas = false;
	font_cfg.OversampleH = 3;
	font_cfg.OversampleV = 1;
	font_cfg.GlyphMaxAdvanceX = FLT_MAX;
	font_cfg.RasterizerMultiply = 1.1f;
	font_cfg.EllipsisChar = (ImWchar) -1;
	ImFontAtlas_AddFontFromMemoryTTF(igGetIO()->Fonts, font_jura_regular, font_jura_regular_size, 24.f, &font_cfg, NULL);
}
