#include <stdio.h>
#include <stdlib.h>
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <glad/glad.h>
#include <cimgui.h>
#include <cimgui_impl.h>
#include <GLFW/glfw3.h>
#include "../libra/util.h"
#include "../libra/dat_container.h"
#include "../libra/model.h"
#include "renderer.h"

typedef struct {
	vec2 rot;
	f32 zoom;
} ViewParams;

GLFWwindow* window;
int window_width;
int window_height;
f32 delta_time;
ViewParams view;

static void startup();
static void draw_gui();
static void load_model(RA_Model* model_dest, u8** data_dest, const char* path);
static void draw_model(RenderModel* model, ViewParams* params);
static void shutdown();

int main(int argc, char** argv) {
	if(argc != 2) {
		printf("usage: ./modeleditor <path to .model file>\n");
		return 1;
	}
	
	RA_dat_init();
	startup();
	renderer_init();
	
	RA_Model model;
	u8* model_data;
	
	load_model(&model, &model_data, argv[Y]);
	RenderModel render_model = renderer_upload_model(&model);
	
	view.rot[X] = 0.f;
	view.rot[Y] = 0.f;
	view.zoom = 0.5f;
	
	f32 prev_time = glfwGetTime();
	while(!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		igNewFrame();
		draw_gui();
		igRender();
		glfwMakeContextCurrent(window);
		glfwGetFramebufferSize(window, &window_width, &window_height);
		glViewport(0, 0, window_width, window_height);
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);
		draw_model(&render_model, &view);
		ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());
		glfwSwapBuffers(window);
		
		f32 time = glfwGetTime();
		delta_time = prev_time - time;
		prev_time = time;
	}
	shutdown();
}

static void startup() {
	if(!glfwInit()) {
		fprintf(stderr, "Failed to load GLFW.\n");
		abort();
	}
	
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	
	window = glfwCreateWindow(1280, 720, "RA Model Editor", NULL, NULL);
	if(!window) {
		fprintf(stderr, "Failed to open GLFW window.\n");
		abort();
	}
	
	glfwMakeContextCurrent(window);
	
	glfwSwapInterval(1);
	
	if(gladLoadGL() == 0) {
		fprintf(stderr, "Failed to load OpenGL.\n");
		abort();
	}
	
	igCreateContext(NULL);
	
	ImGui_ImplGlfw_InitForOpenGL(window, true);
 	ImGui_ImplOpenGL3_Init("#version 430");
	
	igStyleColorsDark(NULL);
}

static void draw_gui() {
	ImVec2 zero = {0, 0};
	igSetNextWindowPos(zero, ImGuiCond_Always, zero);
	ImVec2 gui_size = {200.f, window_height};
	igSetNextWindowSize(gui_size, ImGuiCond_Always);
	igBegin("gui", NULL, ImGuiWindowFlags_NoDecoration);
	igSliderFloat("Zoom", &view.zoom, 0.01f, 1.f, "%.2f", ImGuiSliderFlags_None);
	igEnd();
}

static void load_model(RA_Model* model_dest, u8** data_dest, const char* path) {
	RA_Result result;
	
	u8* file_data;
	s32 file_size;
	if((result = RA_read_entire_file(&file_data, &file_size, path)) != RA_SUCCESS) {
		fprintf(stderr, "Failed to read file '%s' (%s).\n", path, result);
		abort();
	}
	
	RA_DatFile dat;
	if((result = RA_parse_dat_file(&dat, file_data, file_size)) != RA_SUCCESS) {
		fprintf(stderr, "Failed to parse header for file '%s' (%s).\n", path, result);
		abort();
	}
	
	if((result = RA_parse_model(model_dest, &dat)) != RA_SUCCESS) {
		fprintf(stderr, "Failed to parse model file '%s' (%s).\n", path, result);
		abort();
	}
	
	*data_dest = file_data;
}

static void draw_model(RenderModel* model, ViewParams* params) {
	vec2 view_size = {window_width, window_height};
	
	vec3 bb_centre = {0.f, 0.08f, 0.f};
	vec3 bb_size = {1.f, 1.f, 1.f};
	
	f32 camera_distance = 0.5f;
	
	vec3 eye = {(camera_distance * (2.f - params->zoom * 1.9f)), 0, 0};
	vec3 centre = {0, 0, 0};
	vec3 up = {0, 1, 0};
	mat4x4 view_fixed;
	mat4x4_look_at(view_fixed, eye, centre, up);
	
	mat4x4 view_pitched;
	mat4x4_rotate_Z(view_pitched, view_fixed, params->rot[Y]);
	
	mat4x4 view_yawed;
	mat4x4_rotate_Y(view_yawed, view_pitched, params->rot[X]);
	
	mat4x4 trans;
	mat4x4_translate(trans, -bb_centre[X], -bb_centre[Y], -bb_centre[Z]);
	
	mat4x4 view;
	mat4x4_mul(view, view_yawed, trans);
	
	mat4x4 proj;
	mat4x4_perspective(proj, M_PI * 0.5f, view_size[X] / view_size[Y], 0.1f, 10000.0f);
	
	renderer_draw_model(model, view, proj);
	
	static bool is_dragging = false;
	
	ImGuiIO* io = igGetIO();
	vec2 mouse_delta = {io->MouseDelta.x * 0.01f, io->MouseDelta.y * -0.01f};
	
	ImVec2 pos;
	igGetMousePos(&pos);
	b8 image_hovered = pos.x > 200;
	
	if(image_hovered || is_dragging) {
		if(igIsMouseDragging(ImGuiMouseButton_Left, 0.f)) {
			is_dragging = true;
			params->rot[X] += mouse_delta[X];
			params->rot[Y] += mouse_delta[Y];
		}
		
		params->zoom *= io->MouseWheel * delta_time * 0.0001 + 1;
		if(params->zoom < 0.f) params->zoom = 0.f;
		if(params->zoom > 1.f) params->zoom = 1.f;
	}
	
	if(igIsMouseReleased_ID(ImGuiPopupFlags_MouseButtonLeft, 0)) {
		is_dragging = false;
	}
}

static void shutdown() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	igDestroyContext(NULL);
	
	glfwDestroyWindow(window);
	glfwTerminate();
}
