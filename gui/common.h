#ifndef GUI_COMMON_H
#define GUI_COMMON_H

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <glad/glad.h>
#include <cimgui.h>
#include <cimgui_impl.h>
#include <GLFW/glfw3.h>

#include "../libra/util.h"

#ifdef __cplusplus
extern "C" {
#endif

GLFWwindow* GUI_startup(const char* window_title, s32 window_width, s32 window_height);
void GUI_main_loop(GLFWwindow* window, void (*update)(f32 frame_time));
void GUI_shutdown(GLFWwindow* window);

#ifdef __cplusplus
}
#endif

#endif
