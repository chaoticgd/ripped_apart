#include "model.h"

RA_Result RA_model_parse(RA_Model* model, u8* data, u32 size) {
	RA_Result result;
	
	memset(model, 0, sizeof(RA_Model));
	RA_arena_create(&model->arena);
	
	model->file_data = data;
	model->file_size = size;
	
	RA_DatFile dat;
	if((result = RA_dat_parse(&dat, data, size, 0)) != RA_SUCCESS) {
		return result;
	}
	
	RA_DatLump* look = RA_dat_lookup_lump(&dat, LUMP_MODEL_LOOK);
	if(look == NULL) {
		RA_dat_free(&dat, DONT_FREE_FILE_DATA);
		RA_arena_destroy(&model->arena);
		return RA_FAILURE("missing model look lump");
	}
	model->looks = (RA_ModelLook*) look->data;
	model->look_count = look->size / sizeof(RA_ModelLook);

	for(u32 i = 0; i < dat.lump_count; i++) {
		RA_DatLump* lump = &dat.lumps[i];
		if(lump->type_crc == LUMP_MODEL_LOOK) {
			model->looks = (RA_ModelLook*) lump->data;
			model->look_count = lump->size / sizeof(RA_ModelLook);
		} else if(lump->type_crc == LUMP_MODEL_BUILT) {
			model->built = (RA_ModelBuilt*) lump->data;
		} else if(lump->type_crc == LUMP_MODEL_MATERIAL) {
			u64* src = (u64*) lump->data;
			u32 count = lump->size / 32;
			model->materials = RA_arena_alloc(&model->arena, count * sizeof(RA_ModelMaterial));
			model->material_count = count;
			for(u32 j = 0; j < count; j++) {
				u64 path_offset = *src++;
				u64 name_offset = *src++;
				model->materials[j].path = (char*) dat.file_data + path_offset;
				model->materials[j].name = (char*) dat.file_data + name_offset;
			}
		} else if(lump->type_crc == LUMP_MODEL_SUBSET) {
			model->subsets = (RA_ModelSubset*) lump->data;
			model->subset_count = lump->size / sizeof(RA_ModelSubset);
		} else if(lump->type_crc == LUMP_MODEL_INDEX) {
			model->indices = (u16*) lump->data;
			model->index_count = lump->size / sizeof(u16);
		} else if(lump->type_crc == LUMP_MODEL_STD_VERT) {
			model->std_verts = (RA_ModelStdVert*) lump->data;
			model->std_vert_count = lump->size / sizeof(RA_ModelStdVert);
		}
	}
	
	RA_dat_free(&dat, DONT_FREE_FILE_DATA);
	
	return RA_SUCCESS;
}
