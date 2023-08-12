#include "material.h"

typedef struct {
	/* 0x00 */ u32 lump_size;
	/* 0x04 */ u32 first_array_count;
	/* 0x08 */ u32 float_array_offset;
	/* 0x0c */ u32 unknown_c;
	/* 0x10 */ u32 unknown_10;
	/* 0x14 */ u32 texture_count;
	/* 0x18 */ u32 texture_table_offset;
	/* 0x1c */ u32 texture_strings_offset;
} MaterialTextureLumpTypeAHeader;

typedef struct {
	/* 0x0 */ u32 string_offset;
	/* 0x4 */ u32 unknown_crc;
} TextureTableEntry;

typedef struct {
	/* 0x0 */ u32 unknown_0;
	/* 0x4 */ u32 unknown_4;
	/* 0x8 */ u32 unknown_8;
	/* 0xc */ u32 unknown_c;
} MaterialTextureLumpTypeBHeader;

RA_Result RA_material_parse(RA_Material* material, const RA_DatFile* dat, const char* path) {
	u32 first_crc = 0xe1275683;
	u32 textures_a_crc = 0xf5260180;
	u32 textures_b_crc = 0xd9b12454;
	
	memset(material, 0, sizeof(RA_Material));
	material->file_data = dat->file_data;
	material->file_size = dat->file_size;
	RA_arena_create(&material->arena);
	
	char* path_copy = RA_arena_alloc(&material->arena, strlen(path) + 1);
	strcpy(path_copy, path);
	material->file_path = path_copy;
	
	b8 has_first_lump = false;
	b8 has_textures_lump = false;
	
	for(u32 i = 0; i < dat->lump_count; i++) {
		RA_DatLump* lump = &dat->lumps[i];
		if(lump->type_crc == first_crc) {
			has_first_lump = true;
		} else if(lump->type_crc == textures_a_crc) {
			MaterialTextureLumpTypeAHeader* header = (MaterialTextureLumpTypeAHeader*) dat->lumps[i].data;
			TextureTableEntry* textures = (TextureTableEntry*) (dat->lumps[i].data + header->texture_table_offset);
			material->textures = RA_arena_alloc(&material->arena, header->texture_count * sizeof(RA_MaterialTexture));
			material->texture_count = header->texture_count;
			for(u32 j = 0; j < header->texture_count; j++) {
				TextureTableEntry* texture = &textures[j];
				material->textures[j].texture_path = (const char*) (dat->lumps[i].data + header->texture_strings_offset + texture->string_offset);
				material->textures[j].type = texture->unknown_crc;
			}
			has_textures_lump = true;
		} else if(lump->type_crc == textures_b_crc) {
			// TODO: Parse the old format.
			material->texture_count = 0;
			has_textures_lump = true;
		}
	}
	
	if(!(has_first_lump && has_textures_lump)) {
		RA_arena_destroy(&material->arena);
		return RA_FAILURE("insufficient lumpology");
	}
	
	return RA_SUCCESS;
}
