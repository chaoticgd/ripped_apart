#include "model.h"

RA_Result RA_parse_model(RA_Model* model, RA_DatFile* dat) {
	u32 subset_crc = RA_crc_string("Model Subset");
	u32 index_crc = RA_crc_string("Model Index");
	u32 std_vert_crc = RA_crc_string("Model Std Vert");
	
	memset(model, 0, sizeof(RA_Model));
	for(s32 i = 0; i < dat->lump_count; i++) {
		RA_DatLump* lump = &dat->lumps[i];
		if(lump->type_crc == subset_crc) {
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
