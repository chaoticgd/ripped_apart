#include "dependency_dag.h"

#include "dat_container.h"

RA_Result RA_dag_parse(RA_DependencyDag* dag, u8* data, u32 size) {
	u32 file_paths_crc = 0xd101a6cc;
	u32 asset_thing_crc = 0xf958372e;
	
	RA_Result result;
	
	memset(dag, 0, sizeof(RA_DependencyDag));
	RA_arena_create(&dag->arena);
	
	dag->file_data = data;
	dag->file_size = size;
	
	RA_DatFile dat;
	if((result = RA_dat_parse(&dat, data + sizeof(RA_DependencyDagFileHeader), size - sizeof(RA_DependencyDagFileHeader))) != RA_SUCCESS) {
		return result;
	}
	
	b8 has_file_paths_lump = false;
	b8 has_asset_thing_lump = false;
	
	for(u32 i = 0; i < dat.lump_count; i++) {
		RA_DatLump* lump = &dat.lumps[i];
		if(lump->type_crc == file_paths_crc) {
			if(dag->assets == NULL) {
				dag->assets = RA_arena_calloc(&dag->arena, lump->size / 4, sizeof(RA_DependencyDagAsset));
				dag->asset_count = lump->size / 4;
			} else if(dag->asset_count != lump->size / 4) {
				return "file path count mismatch";
			}
			u32* path_offsets = (u32*) lump->data;
			for(u32 i = 0; i < dag->asset_count; i++) {
				dag->assets[i].path = (char*) (data + sizeof(RA_DependencyDagFileHeader) + path_offsets[i]);
			}
			has_file_paths_lump = true;
		} else if(lump->type_crc == asset_thing_crc) {
			if(dag->assets == NULL) {
				dag->assets = RA_arena_calloc(&dag->arena, lump->size / 4, sizeof(RA_DependencyDagAsset));
				dag->asset_count = lump->size / 4;
			} else if(dag->asset_count != lump->size / 4) {
				return "asset thing count mismatch";
			}
			s32* things = (s32*) lump->data;
			for(u32 i = 0; i < dag->asset_count; i++) {
				dag->assets[i].thing = things[i];
			}
			has_asset_thing_lump = true;
		}
	}
	
	RA_dat_free(&dat, false);
	
	if(!(has_file_paths_lump && has_asset_thing_lump)) {
		RA_arena_destroy(&dat.arena);
		return "insufficient lumpology";
	}
	
	return RA_SUCCESS;
}

RA_Result RA_dag_build(RA_DependencyDag* dag, u8** data_dest, u32* size_dest) {
	return RA_SUCCESS;
}

RA_Result RA_dag_free(RA_DependencyDag* dag, b8 free_file_data) {
	RA_arena_destroy(&dag->arena);
	if(free_file_data && dag->file_data) {
		free(dag->file_data);
	}
	return RA_SUCCESS;
}
