#include "../libra/util.h"
#include "../libra/dat_container.h"
#include "../libra/model.h"
#include "../libra/material.h"
#include "../gui/common.h"
#include "../gui/menu.h"
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
f32 gui_to_preview_ratio = 0.5f;

u8* model_file_data;
u32 model_file_size;
RA_Model model;
RenderModel render_model;
RA_Material* materials;
u32 selected_material = 0;

static void update(f32 frame_time);
static void draw_gui();
static void load_model(const char* path, const char* root_asset_dir);
static void draw_model(RenderModel* model, ViewParams* params);

#define _X 0
#define _Y 1
#define _Z 2
#define _W 3

int main(int argc, char** argv) {
	if(argc != 3) {
		printf("usage: ./modeleditor <path to .model file> <root asset directory>\n");
		return 1;
	}
	
	window = GUI_startup("Ripped Apart Model Editor", 1280, 720);
	renderer_init();
	
	load_model(argv[1], argv[2]);
	render_model = renderer_upload_model(&model);
	
	view.rot[_X] = 0.f;
	view.rot[_Y] = 0.f;
	view.zoom = 0.5f;
	
	GUI_main_loop(window, update);
	GUI_shutdown(window);
}

static void update(f32 frame_time) {
	draw_gui();
	igRender();
	glfwMakeContextCurrent(window);
	glfwGetFramebufferSize(window, &window_width, &window_height);
	gui_to_preview_ratio = 640.f / window_width;
	glViewport(0, 0, window_width * (1.f + gui_to_preview_ratio), window_height);
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	draw_model(&render_model, &view);
}

static void draw_gui() {
	ImVec2 eight_hundred_by_zero = {800, 0};
	GUI_menu_begin(640.f, window_height);
	if(GUI_menu_tab("Home")) {
		igSliderFloat("Zoom", &view.zoom, 0.01f, 1.f, "%.2f", ImGuiSliderFlags_None);
	}
	if(model.joints != NULL && GUI_menu_tab("Joints")) {
		for(u32 i = 0; i < model.joint_count; i++) {
			igText(model.joints[i].name.string);
		}
	}
	if(model.locators != NULL && GUI_menu_tab("Locators")) {
		for(u32 i = 0; i < model.locator_count; i++) {
			igText(model.locators[i].name.string);
		}
	}
	if(model.spline_subsets != NULL && GUI_menu_tab("Spline Subsets")) {
		for(u32 i = 0; i < model.spline_subset_count; i++) {
			igText(model.spline_subsets[i].name.string);
		}
	}
	if(GUI_menu_tab("Materials")) {
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
	GUI_menu_end();
}

static void load_model(const char* path, const char* root_asset_dir) {
	RA_Result result;
	
	if((result = RA_file_read(&model_file_data, &model_file_size, path)) != RA_SUCCESS) {
		fprintf(stderr, "error: Failed to read model file '%s' (%s).\n", path, result->message);
		abort();
	}
	
	if((result = RA_model_parse(&model, model_file_data, model_file_size)) != RA_SUCCESS) {
		fprintf(stderr, "error: Failed to parse model file '%s' (%s).\n", path, result->message);
		abort();
	}
	
	materials = calloc(model.material_count, sizeof(RA_Material));
	for(u32 i = 0; i < model.material_count; i++) {
		RA_ModelMaterial* material = &model.materials[i];
		printf("material %s: %s\n", material->name, material->path);
		
		if(strlen(material->path) > 0) {
			char material_path[1024];
			snprintf(material_path, 1024, "%s/%s", root_asset_dir, material->path);
			RA_file_fix_path(&material_path[strlen(root_asset_dir)]);
			
			u8* material_data;
			u32 material_size;
			if((result = RA_file_read(&material_data, &material_size, material_path)) != RA_SUCCESS) {
				fprintf(stderr, "error: Failed to read material file '%s' (%s).\n", material_path, result->message);
				abort();
			}
			
			RA_DatFile dat;
			if((result = RA_dat_parse(&dat, material_data, material_size, 0)) != RA_SUCCESS) {
				fprintf(stderr, "error: Failed to parse header for material file '%s' (%s).\n", material_path, result->message);
				abort();
			}
			
			if((result = RA_material_parse(&materials[i], &dat, material->path)) != RA_SUCCESS) {
				fprintf(stderr, "error: Failed to parse material file '%s' (%s).\n", material_path, result->message);
				abort();
			}
		}
	}
}

static void draw_model(RenderModel* model, ViewParams* params) {
	vec2 view_size = {window_width * (1.f + gui_to_preview_ratio), window_height};
	
	vec3 bb_centre = {0.f, 0.08f, 0.f};
	vec3 bb_size = {1.f, 1.f, 1.f};
	
	f32 camera_distance = 0.5f;
	
	vec3 eye = {(camera_distance * (2.f - params->zoom * 1.9f)), 0, 0};
	vec3 centre = {0, 0, 0};
	vec3 up = {0, 1, 0};
	mat4x4 view_fixed;
	mat4x4_look_at(view_fixed, eye, centre, up);
	
	mat4x4 view_pitched;
	mat4x4_rotate_Z(view_pitched, view_fixed, params->rot[_Y]);
	
	mat4x4 view_yawed;
	mat4x4_rotate_Y(view_yawed, view_pitched, params->rot[_X]);
	
	mat4x4 trans;
	mat4x4_translate(trans, -bb_centre[_X], -bb_centre[_Y], -bb_centre[_Z]);
	
	mat4x4 view;
	mat4x4_mul(view, view_yawed, trans);
	
	mat4x4 proj;
	mat4x4_perspective(proj, RA_PI * 0.5f, view_size[_X] / view_size[_Y], 0.01f, 1000.0f);
	
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
			params->rot[_X] += mouse_delta[_X];
			params->rot[_Y] += mouse_delta[_Y];
		}
		
		params->zoom *= io->MouseWheel * delta_time * 0.0001 + 1;
		if(params->zoom < 0.f) params->zoom = 0.f;
		if(params->zoom > 1.f) params->zoom = 1.f;
	}
	
	if(igIsMouseReleased_ID(ImGuiPopupFlags_MouseButtonLeft, 0)) {
		is_dragging = false;
	}
}
