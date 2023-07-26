#ifndef LIBRA_MATERIAL_H
#define LIBRA_MATERIAL_H

#include "util.h"
#include "arena.h"
#include "dat_container.h"

typedef struct {
	const char* texture_path;
	u32 type;
} RA_MaterialTexture;

typedef struct {
	u8* file_data;
	u32 file_size;
	RA_Arena arena;
	const char* file_path;
	RA_MaterialTexture* textures;
	u32 texture_count;
} RA_Material;

RA_Result RA_material_parse(RA_Material* material, const RA_DatFile* dat, const char* path);

#endif
