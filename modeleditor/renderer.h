#ifndef MODELEDITOR_RENDERER_H
#define MODELEDITOR_RENDERER_H

#include <glad/glad.h>
#include <linmath.h>

#include "../libra/model.h"

typedef struct {
	GLuint vertex_array_object;
	GLuint vertex_buffer;
	GLuint index_buffer;
	u32 vertex_count;
	u32 index_count;
} RenderModelSubset;

typedef struct {
	RenderModelSubset* subsets;
	u32 subset_count;
} RenderModel;

void renderer_init();
RenderModel renderer_upload_model(RA_Model* model);
void renderer_draw_model(const RenderModel* model, const mat4x4 view, const mat4x4 proj);

#endif
