#include "common.h"

#ifndef WIN32
#include <unistd.h>
#endif

static void setup_style(ImGuiStyle* dst);

GLFWwindow* GUI_startup(const char* window_title, s32 window_width, s32 window_height) {
	if(!glfwInit()) {
		fprintf(stderr, "error: Failed to load GLFW.\n");
		abort();
	}
	
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	
	GLFWwindow* window = glfwCreateWindow(window_width, window_height, window_title, NULL, NULL);
	if(window == NULL) {
		GUI_message_box("Failed to open GLFW window.", window_title, GUI_MESSAGE_BOX_ERROR);
		abort();
	}
	
	glfwMakeContextCurrent(window);
	
	glfwSwapInterval(1);
	
	if(gladLoadGL() == 0) {
		GUI_message_box("Failed to load OpenGL.", window_title, GUI_MESSAGE_BOX_ERROR);
		abort();
	}
	
	igCreateContext(NULL);
	
	ImGui_ImplGlfw_InitForOpenGL(window, true);
 	ImGui_ImplOpenGL3_Init("#version 430");
	
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

void GUI_message_box(const char* text, const char* title, MessageBoxType type) {
	fprintf(stderr, "error: %s\n", text);
#ifdef WIN32
	MessageBoxA(NULL, text, title, MB_OK);
#else
	if(fork() == 0) {
		char zenity_type[32];
		switch(type) {
			case GUI_MESSAGE_BOX_INFO: RA_string_copy(zenity_type, "--info", sizeof(zenity_type)); break;
			case GUI_MESSAGE_BOX_ERROR: RA_string_copy(zenity_type, "--error", sizeof(zenity_type)); break;
		}
		
		char zenity_text[1024];
		snprintf(zenity_text, sizeof(zenity_text), "--text=%s", text);
		
		char zenity_title[1024];
		snprintf(zenity_title, sizeof(zenity_title), "--title=%s", title);
		
		char* args[] = {
			"/usr/bin/env",
			"zenity",
			zenity_type,
			zenity_text,
			zenity_title,
			NULL
		};
		execv(args[0], args);
	}
#endif
}

static void setup_style(ImGuiStyle* dst) {
    ImGuiStyle* style = dst ? dst : igGetStyle();
    ImVec4* colors = style->Colors;

    colors[ImGuiCol_Text]                   = (ImVec4) {1.00f, 1.00f, 1.00f, 1.00f};
    colors[ImGuiCol_TextDisabled]           = (ImVec4) {0.50f, 0.50f, 0.50f, 1.00f};
    colors[ImGuiCol_WindowBg]               = (ImVec4) {0.06f, 0.06f, 0.06f, 0.94f};
    colors[ImGuiCol_ChildBg]                = (ImVec4) {0.00f, 0.00f, 0.00f, 0.00f};
    colors[ImGuiCol_PopupBg]                = (ImVec4) {0.08f, 0.08f, 0.08f, 0.94f};
    colors[ImGuiCol_Border]                 = (ImVec4) {0.43f, 0.43f, 0.50f, 0.50f};
    colors[ImGuiCol_BorderShadow]           = (ImVec4) {0.00f, 0.00f, 0.00f, 0.00f};
    colors[ImGuiCol_FrameBg]                = (ImVec4) {0.16f, 0.29f, 0.48f, 0.54f};
    colors[ImGuiCol_FrameBgHovered]         = (ImVec4) {0.26f, 0.59f, 0.98f, 0.40f};
    colors[ImGuiCol_FrameBgActive]          = (ImVec4) {0.26f, 0.59f, 0.98f, 0.67f};
    colors[ImGuiCol_TitleBg]                = (ImVec4) {0.04f, 0.04f, 0.04f, 1.00f};
    colors[ImGuiCol_TitleBgActive]          = (ImVec4) {0.16f, 0.29f, 0.48f, 1.00f};
    colors[ImGuiCol_TitleBgCollapsed]       = (ImVec4) {0.00f, 0.00f, 0.00f, 0.51f};
    colors[ImGuiCol_MenuBarBg]              = (ImVec4) {0.14f, 0.14f, 0.14f, 1.00f};
    colors[ImGuiCol_ScrollbarBg]            = (ImVec4) {0.02f, 0.02f, 0.02f, 0.53f};
    colors[ImGuiCol_ScrollbarGrab]          = (ImVec4) {0.31f, 0.31f, 0.31f, 1.00f};
    colors[ImGuiCol_ScrollbarGrabHovered]   = (ImVec4) {0.41f, 0.41f, 0.41f, 1.00f};
    colors[ImGuiCol_ScrollbarGrabActive]    = (ImVec4) {0.51f, 0.51f, 0.51f, 1.00f};
    colors[ImGuiCol_CheckMark]              = (ImVec4) {0.26f, 0.59f, 0.98f, 1.00f};
    colors[ImGuiCol_SliderGrab]             = (ImVec4) {0.24f, 0.52f, 0.88f, 1.00f};
    colors[ImGuiCol_SliderGrabActive]       = (ImVec4) {0.26f, 0.59f, 0.98f, 1.00f};
    colors[ImGuiCol_Button]                 = (ImVec4) {0.26f, 0.59f, 0.98f, 0.40f};
    colors[ImGuiCol_ButtonHovered]          = (ImVec4) {0.26f, 0.59f, 0.98f, 1.00f};
    colors[ImGuiCol_ButtonActive]           = (ImVec4) {0.06f, 0.53f, 0.98f, 1.00f};
    colors[ImGuiCol_Header]                 = (ImVec4) {0.26f, 0.59f, 0.98f, 0.31f};
    colors[ImGuiCol_HeaderHovered]          = (ImVec4) {0.26f, 0.59f, 0.98f, 0.80f};
    colors[ImGuiCol_HeaderActive]           = (ImVec4) {0.26f, 0.59f, 0.98f, 1.00f};
    colors[ImGuiCol_Separator]              = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered]       = (ImVec4) {0.10f, 0.40f, 0.75f, 0.78f};
    colors[ImGuiCol_SeparatorActive]        = (ImVec4) {0.10f, 0.40f, 0.75f, 1.00f};
    colors[ImGuiCol_ResizeGrip]             = (ImVec4) {0.26f, 0.59f, 0.98f, 0.20f};
    colors[ImGuiCol_ResizeGripHovered]      = (ImVec4) {0.26f, 0.59f, 0.98f, 0.67f};
    colors[ImGuiCol_ResizeGripActive]       = (ImVec4) {0.26f, 0.59f, 0.98f, 0.95f};
    igImLerp_Vec4(&colors[ImGuiCol_Tab], colors[ImGuiCol_Header], colors[ImGuiCol_TitleBgActive], 0.80f);
    colors[ImGuiCol_TabHovered]             = colors[ImGuiCol_HeaderHovered];
    igImLerp_Vec4(&colors[ImGuiCol_TabActive], colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
    igImLerp_Vec4(&colors[ImGuiCol_TabUnfocused], colors[ImGuiCol_Tab], colors[ImGuiCol_TitleBg], 0.80f);
    igImLerp_Vec4(&colors[ImGuiCol_TabUnfocusedActive], colors[ImGuiCol_TabActive], colors[ImGuiCol_TitleBg], 0.40f);
    colors[ImGuiCol_DockingPreview]         = colors[ImGuiCol_HeaderActive]; // * ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
    colors[ImGuiCol_DockingEmptyBg]         = (ImVec4) {0.20f, 0.20f, 0.20f, 1.00f};
    colors[ImGuiCol_PlotLines]              = (ImVec4) {0.61f, 0.61f, 0.61f, 1.00f};
    colors[ImGuiCol_PlotLinesHovered]       = (ImVec4) {1.00f, 0.43f, 0.35f, 1.00f};
    colors[ImGuiCol_PlotHistogram]          = (ImVec4) {0.90f, 0.70f, 0.00f, 1.00f};
    colors[ImGuiCol_PlotHistogramHovered]   = (ImVec4) {1.00f, 0.60f, 0.00f, 1.00f};
    colors[ImGuiCol_TableHeaderBg]          = (ImVec4) {0.19f, 0.19f, 0.20f, 1.00f};
    colors[ImGuiCol_TableBorderStrong]      = (ImVec4) {0.31f, 0.31f, 0.35f, 1.00f};   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableBorderLight]       = (ImVec4) {0.23f, 0.23f, 0.25f, 1.00f};   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableRowBg]             = (ImVec4) {0.00f, 0.00f, 0.00f, 0.00f};
    colors[ImGuiCol_TableRowBgAlt]          = (ImVec4) {1.00f, 1.00f, 1.00f, 0.06f};
    colors[ImGuiCol_TextSelectedBg]         = (ImVec4) {0.26f, 0.59f, 0.98f, 0.35f};
    colors[ImGuiCol_DragDropTarget]         = (ImVec4) {1.00f, 1.00f, 0.00f, 0.90f};
    colors[ImGuiCol_NavHighlight]           = (ImVec4) {0.26f, 0.59f, 0.98f, 1.00f};
    colors[ImGuiCol_NavWindowingHighlight]  = (ImVec4) {1.00f, 1.00f, 1.00f, 0.70f};
    colors[ImGuiCol_NavWindowingDimBg]      = (ImVec4) {0.80f, 0.80f, 0.80f, 0.20f};
    colors[ImGuiCol_ModalWindowDimBg]       = (ImVec4) {0.80f, 0.80f, 0.80f, 0.35f};
}
