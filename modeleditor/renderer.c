#include "renderer.h"

static void build_shaders();
static GLuint build_shader(const char* vertex_source, const char* fragment_source, GLuint** uniforms_dest, const char** uniform_names);

static GLuint program;
static GLuint view_matrix_uniform;
static GLuint proj_matrix_uniform;

static void
error_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
	static b8 had_errors = false;
	if(type == GL_DEBUG_TYPE_OTHER && !had_errors) {
		return;
	}
	
	const char* type_string = "Unexpected OpenGL Message";
	switch(type) {
		case GL_DEBUG_TYPE_ERROR: type_string = "OpenGL Error"; had_errors = true; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: type_string = "OpenGL Deprecation Warning"; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: type_string = "OpenGL Undefined Behaviour Warning"; break;
		case GL_DEBUG_TYPE_PORTABILITY: type_string = "OpenGL Portability Warning"; break;
		case GL_DEBUG_TYPE_PERFORMANCE: type_string = "OpenGL Performance Warning"; break;
		case GL_DEBUG_TYPE_OTHER: type_string = "OpenGL Message"; break;
	}
	
	fprintf(stderr, "%s: type = 0x%x, severity = 0x%x, message = %s\n",
		type_string, type, severity, message);
}

void renderer_init() {
	build_shaders();
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(error_callback, 0);
}

RenderModel renderer_upload_model(RA_Model* model) {
	RenderModel render_model;
	
	render_model.subsets = malloc(model->subset_count * sizeof(RenderModelSubset));
	render_model.subset_count = model->subset_count;
	
	for(u32 i = 0; i < model->subset_count; i++) {
		RA_ModelSubset* subset = &model->subsets[i];
		RenderModelSubset* render_subset = &render_model.subsets[i];
		
		// Setup vertex array object.
		glGenVertexArrays(1, &render_subset->vertex_array_object);
		glBindVertexArray(render_subset->vertex_array_object);
		
		// Upload buffers.
		glGenBuffers(1, &render_subset->vertex_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, render_subset->vertex_buffer);
		glBufferData(GL_ARRAY_BUFFER, subset->vertex_count * sizeof(RA_ModelStdVert), &model->std_verts[subset->vertex_begin], GL_STATIC_DRAW);
		render_subset->vertex_count = subset->vertex_count;
		
		glGenBuffers(1, &render_subset->index_buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, render_subset->index_buffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, model->index_count * sizeof(u16), &model->indices[subset->index_begin], GL_STATIC_DRAW);
		render_subset->index_count = subset->index_count;
		
		// Declare vertex buffer layout.
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 4, GL_SHORT, GL_TRUE, sizeof(RA_ModelStdVert), (void*) offsetof(RA_ModelStdVert, pos_x));
		glEnableVertexAttribArray(1);
		glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, sizeof(RA_ModelStdVert), (void*) offsetof(RA_ModelStdVert, normals));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_SHORT, GL_TRUE, sizeof(RA_ModelStdVert), (void*) offsetof(RA_ModelStdVert, texcoord_u));
	}
	
	return render_model;
}

void renderer_draw_model(const RenderModel* model, const mat4x4 view, const mat4x4 proj) {
	glUniformMatrix4fv(view_matrix_uniform, 1, GL_FALSE, &view[0][0]);
	glUniformMatrix4fv(proj_matrix_uniform, 1, GL_FALSE, &proj[0][0]);
	for(u32 i = 0; i < model->subset_count; i++) {
		const RenderModelSubset* subset = &model->subsets[i];
		glBindVertexArray(subset->vertex_array_object);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, subset->index_buffer);
		glDrawElements(GL_TRIANGLES, subset->index_count, GL_UNSIGNED_SHORT, NULL);
	}
}

#define GLSL(...) "#version 430\n" #__VA_ARGS__

static const char* main_vertex_source = GLSL(
	uniform mat4 view_matrix;
	uniform mat4 proj_matrix;
	layout(location = 0) in vec4 pos;
	layout(location = 1) in vec4 norm;
	layout(location = 2) in vec4 uv;
	
	void main() {
		gl_Position = proj_matrix * view_matrix * vec4(pos.xyz, 1);
	}
);

static const char* main_fragment_source = GLSL(
	out vec4 col;
	
	void main() {
		col = vec4(1, 1, 1, 1);
	}
);

static void build_shaders() {
	GLuint* uniforms[] = {&view_matrix_uniform, &proj_matrix_uniform};
	const char* uniform_names[] = {"view_matrix", "proj_matrix", NULL};
	program = build_shader(main_vertex_source, main_fragment_source, uniforms, uniform_names);
}

static GLuint build_shader(const char* vertex_source, const char* fragment_source, GLuint** uniforms_dest, const char** uniform_names) {
	// Compile shaders.
	GLint result;
	int log_size;
	
	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_source, NULL);
	glCompileShader(vertex_shader);
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &result);
	glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &log_size);
	if(log_size > 0) {
		char* log = malloc(log_size);
		glGetShaderInfoLog(vertex_shader, log_size, NULL, log);
		fprintf(stderr, "error: Failed to compile vertex shader!\n%s", log);
		abort();
	}
	
	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_source, NULL);
	glCompileShader(fragment_shader);
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &result);
	glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &log_size);
	if(log_size > 0) {
		char* log = malloc(log_size);
		glGetShaderInfoLog(fragment_shader, log_size, NULL, log);
		fprintf(stderr, "error: Failed to compile fragment shader!\n%s", log);
		abort();
	}
	
	// Link shaders.
	GLuint program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	
	glLinkProgram(program);
	while(*uniform_names != NULL) {
		**uniforms_dest = glGetUniformLocation(program, *uniform_names);
		uniforms_dest++;
		uniform_names++;
	}
	glUseProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &result);
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_size);
	if(log_size > 0) {
		char* log = malloc(log_size);
		glGetShaderInfoLog(program, log_size, NULL, log);
		fprintf(stderr, "error: Failed to link shaders!\n%s", log);
		abort();
	}

	glDetachShader(program, vertex_shader);
	glDetachShader(program, fragment_shader);
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	
	return program;
}
