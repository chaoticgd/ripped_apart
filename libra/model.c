#include "model.h"

RA_Result RA_model_parse(RA_Model* model, RA_DatFile* dat) {
	u32 look_crc = RA_crc_string("Model Look");
	u32 built_crc = RA_crc_string("Model Built");
	u32 material_crc = RA_crc_string("Model Material");
	u32 subset_crc = RA_crc_string("Model Subset");
	u32 index_crc = RA_crc_string("Model Index");
	u32 std_vert_crc = RA_crc_string("Model Std Vert");
	
	memset(model, 0, sizeof(RA_Model));
	RA_arena_create(&model->arena);
	
	for(u32 i = 0; i < dat->lump_count; i++) {
		RA_DatLump* lump = &dat->lumps[i];
		if(lump->type_crc == look_crc) {
			model->looks = (RA_ModelLook*) lump->data;
			model->look_count = lump->size / sizeof(RA_ModelLook);
		} else if(lump->type_crc == built_crc) {
			model->built = (RA_ModelBuilt*) lump->data;
		} else if(lump->type_crc == material_crc) {
			u64* src = (u64*) lump->data;
			u32 count = lump->size / 32;
			model->materials = RA_arena_alloc(&model->arena, count * sizeof(RA_ModelMaterial));
			model->material_count = count;
			for(u32 j = 0; j < count; j++) {
				u64 path_offset = *src++;
				u64 name_offset = *src++;
				model->materials[j].path = (char*) dat->file_data + path_offset;
				model->materials[j].name = (char*) dat->file_data + name_offset;
			}
		} else if(lump->type_crc == subset_crc) {
			model->subsets = (RA_ModelSubset*) lump->data;
			model->subset_count = lump->size / sizeof(RA_ModelSubset);
		} else if(lump->type_crc == index_crc) {
			model->indices = (u16*) lump->data;
			model->index_count = lump->size / sizeof(u16);
		} else if(lump->type_crc == std_vert_crc) {
			model->std_verts = (RA_ModelStdVert*) lump->data;
			model->std_vert_count = lump->size / sizeof(RA_ModelStdVert);
		}
	}
	
	return RA_SUCCESS;
}
