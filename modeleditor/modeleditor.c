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
#include "../libra/material.h"
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

u8* model_file_data;
u32 model_file_size;
RA_Model model;
RA_Material* materials;
u32 selected_material = 0;

static void startup();
static void draw_gui();
static void load_model(const char* path, const char* root_asset_dir);
static void draw_model(RenderModel* model, ViewParams* params);
static void shutdown();

int main(int argc, char** argv) {
	if(argc != 3) {
		printf("usage: ./modeleditor <path to .model file> <root asset directory>\n");
		return 1;
	}
	
	RA_dat_init();
	startup();
	renderer_init();
	
	load_model(argv[1], argv[2]);
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
	ImVec2 eight_hundred_by_zero = {800, 0};
	igSetNextWindowPos(zero, ImGuiCond_Always, zero);
	ImVec2 gui_size = {640.f, window_height};
	igSetNextWindowSize(gui_size, ImGuiCond_Always);
	igBegin("gui", NULL, ImGuiWindowFlags_NoDecoration);
	igSliderFloat("Zoom", &view.zoom, 0.01f, 1.f, "%.2f", ImGuiSliderFlags_None);
	if(igCollapsingHeader_BoolPtr("Materials", NULL, ImGuiTreeNodeFlags_None)) {
		const char* preview;
		if(selected_material < model.material_count) {
			preview = materials[selected_material].file_path;
		} else {
			preview = "(no material selected)";
		}
		if(igBeginCombo("Material Path", preview, ImGuiComboFlags_None)) {
			for(u32 i = 0; i < model.material_count; i++) {
				RA_Material* material = &materials[i];
				if(igSelectable_Bool(material->file_path ? material->file_path : "", i == selected_material, ImGuiSelectableFlags_None, eight_hundred_by_zero)) {
					selected_material = i;
				}
			}
			igEndCombo();
		}
		igText("Textures:");
		if(selected_material < model.material_count) {
			for(u32 i = 0; i < materials[selected_material].texture_count; i++) {
				RA_MaterialTexture* texture = &materials[selected_material].textures[i];
				if(igSelectable_Bool(texture->texture_path ? texture->texture_path : "", false, ImGuiSelectableFlags_None, eight_hundred_by_zero)) {
					
				}
			}
		}
	}
	igEnd();
}

static void load_model(const char* path, const char* root_asset_dir) {
	RA_Result result;
	
	if((result = RA_file_read(&model_file_data, &model_file_size, path)) != RA_SUCCESS) {
		fprintf(stderr, "Malfunction while reading model file '%s' (%s).\n", path, result);
		abort();
	}
	
	RA_DatFile dat;
	if((result = RA_dat_parse(&dat, model_file_data, model_file_size)) != RA_SUCCESS) {
		fprintf(stderr, "Malfunction while parsing header for model file '%s' (%s).\n", path, result);
		abort();
	}
	
	if((result = RA_model_parse(&model, &dat)) != RA_SUCCESS) {
		fprintf(stderr, "Malfunction while parsing model file '%s' (%s).\n", path, result);
		abort();
	}
	
	materials = calloc(model.material_count, sizeof(RA_Material));
	for(u32 i = 0; i < model.material_count; i++) {
		RA_ModelMaterial* material = &model.materials[i];
		printf("material %s: %s\n", material->name, material->path);
		
		if(strlen(material->path) > 0) {
			char material_path[1024];
			snprintf(material_path, 1024, "%s/%s", root_asset_dir, material->path);
			
			u8* material_data;
			u32 material_size;
			if((result = RA_file_read(&material_data, &material_size, material_path)) != RA_SUCCESS) {
				fprintf(stderr, "Malfunction while reading material file '%s' (%s).\n", material_path, result);
				abort();
			}
			
			RA_DatFile dat;
			if((result = RA_dat_parse(&dat, material_data, material_size)) != RA_SUCCESS) {
				fprintf(stderr, "Malfunction while parsing header for material file '%s' (%s).\n", material_path, result);
				abort();
			}
			
			if((result = RA_material_parse(&materials[i], &dat, material->path)) != RA_SUCCESS) {
				fprintf(stderr, "Malfunction while parsing material file '%s' (%s).\n", material_path, result);
				abort();
			}
		}
	}
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
	b8 image_hovered = pos.x > 640;
	
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
