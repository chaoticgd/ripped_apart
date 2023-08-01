#include "dependency_dag.h"

#include "dat_container.h"

static RA_Result alloc_assets(RA_DependencyDag* dag, u32 asset_count);

RA_Result RA_dag_parse(RA_DependencyDag* dag, u8* data, u32 size) {
	u32 hashes_crc = 0x933c0d32;
	u32 paths_crc = 0xd101a6cc;
	u32 asset_types_crc = RA_crc_string("Asset Types");
	u32 dependency_indices_crc = 0xf958372e;
	u32 dependency_crc = 0xbc91d1cc;
	u32 unk_crc = 0xbfec699f;
	
	RA_Result result;
	
	memset(dag, 0, sizeof(RA_DependencyDag));
	RA_arena_create(&dag->arena);
	
	dag->file_data = data;
	dag->file_size = size;
	
	RA_DatFile dat;
	if((result = RA_dat_parse(&dat, data + sizeof(RA_DependencyDagFileHeader), size - sizeof(RA_DependencyDagFileHeader))) != RA_SUCCESS) {
		return result;
	}
	
	b8 has_asset_types_lump = false;
	b8 has_hashes_lump = false;
	b8 has_dependencies_lump = false;
	b8 has_file_paths_lump = false;
	b8 has_dependency_index_lump = false;
	
	s32* dependencies = NULL;
	u32 dependency_lump_size = 0;
	
	for(u32 i = 0; i < dat.lump_count; i++) {
		RA_DatLump* lump = &dat.lumps[i];
		if(lump->type_crc == asset_types_crc) {
			if((result = alloc_assets(dag, lump->size)) != RA_SUCCESS) {
				return result;
			}
			for(u32 i = 0; i < dag->asset_count; i++) {
				dag->assets[i].type = lump->data[i];
			}
			has_asset_types_lump = true;
		} else if(lump->type_crc == hashes_crc) {
			if((result = alloc_assets(dag, lump->size / 8)) != RA_SUCCESS) {
				return result;
			}
			u64* hashes = (u64*) lump->data;
			for(u32 i = 0; i < dag->asset_count; i++) {
				dag->assets[i].hash = hashes[i];
			}
			has_hashes_lump = true;
		} else if(lump->type_crc == dependency_crc) {
			dependencies = RA_arena_alloc(&dag->arena, lump->size);
			memcpy(dependencies, lump->data, lump->size);
			dependency_lump_size = lump->size;
			has_dependencies_lump = true;
		} else if(lump->type_crc == paths_crc) {
			if((result = alloc_assets(dag, lump->size / 4)) != RA_SUCCESS) {
				return result;
			}
			u32* path_offsets = (u32*) lump->data;
			for(u32 i = 0; i < dag->asset_count; i++) {
				dag->assets[i].path = (char*) (data + sizeof(RA_DependencyDagFileHeader) + path_offsets[i]);
			}
			has_file_paths_lump = true;
		} else if(lump->type_crc == dependency_indices_crc) {
			if((result = alloc_assets(dag, lump->size / 4)) != RA_SUCCESS) {
				return result;
			}
			s32* dependency_indices = (s32*) lump->data;
			for(u32 i = 0; i < dag->asset_count; i++) {
				s32 dependency_list_index = dependency_indices[i];
				if(dependency_list_index > -1) {
					if(!dependencies) return "no dependencies lump";
					if(dependency_list_index * 4 >= dependency_lump_size) return "dependency list index out of range";
					dag->assets[i].dependencies = (u32*) &dependencies[dependency_list_index];
					s32* dependency = &dependencies[dependency_list_index];
					while(*dependency > -1) {
						if(*dependency >= dag->asset_count) return "dependency index out of range";
						dag->assets[i].dependency_count++;
						dependency++;
					}
				}
			}
			has_dependency_index_lump = true;
		}
	}
	
	RA_dat_free(&dat, false);
	
	if(!(has_asset_types_lump && has_hashes_lump && has_dependencies_lump && has_file_paths_lump && has_dependency_index_lump)) {
		RA_arena_destroy(&dat.arena);
		return "insufficient lumpology";
	}
	
	return RA_SUCCESS;
}

static RA_Result alloc_assets(RA_DependencyDag* dag, u32 asset_count) {
	if(dag->assets == NULL) {
		dag->assets = RA_arena_calloc(&dag->arena, asset_count, sizeof(RA_DependencyDagAsset));
		dag->asset_count = asset_count;
	} else if(dag->asset_count != asset_count) {
		return "asset count mismatch";
	}
	return RA_SUCCESS;
}

RA_Result RA_dag_build(RA_DependencyDag* dag, u8** data_dest, u32* size_dest) {
	u32 hashes_crc = 0x933c0d32;
	u32 paths_crc = 0xd101a6cc;
	u32 asset_types_crc = RA_crc_string("Asset Types");
	u32 dependency_indices_crc = 0xf958372e;
	u32 dependency_crc = 0xbc91d1cc;
	u32 unk_crc = 0xbfec699f;
	
	RA_DatWriter* writer = RA_dat_writer_begin(RA_ASSET_TYPE_DAG, 0xc);
	
	u32 dependency_count = 0;
	for(u32 i = 0; i < dag->asset_count; i++) {
		dependency_count += dag->assets[i].dependency_count + 1;
	}
	
	u64* hashes = RA_dat_writer_lump(writer, hashes_crc, dag->asset_count * 8);
	u32* paths = RA_dat_writer_lump(writer, paths_crc, dag->asset_count * 4);
	u8* asset_types = RA_dat_writer_lump(writer, asset_types_crc, dag->asset_count);
	u32* dependency_indices = RA_dat_writer_lump(writer, dependency_indices_crc, dag->asset_count * 4);
	s32* dependency = RA_dat_writer_lump(writer, dependency_crc, dependency_count * 4);
	u32* unk = RA_dat_writer_lump(writer, unk_crc, 1);
	
	for(u32 i = 0; i < dag->asset_count; i++) {
		asset_types[i] = dag->assets[i].type;
		hashes[i] = dag->assets[i].hash;
		paths[i] = RA_dat_writer_string(writer, dag->assets[i].path);
		for(u32 j = 0; j < dag->assets[i].dependency_count; j++) {
			(*dependency++) = dag->assets[i].dependencies[j];
		}
		(*dependency++) = -1;
	}
	
	RA_dat_writer_finish(writer, data_dest, size_dest);
	return RA_SUCCESS;
}

RA_Result RA_dag_free(RA_DependencyDag* dag, b8 free_file_data) {
	RA_arena_destroy(&dag->arena);
	if(free_file_data && dag->file_data) {
		free(dag->file_data);
	}
	return RA_SUCCESS;
}
