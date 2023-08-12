#include "dependency_dag.h"

#include "dat_container.h"

static RA_Result alloc_assets(RA_DependencyDag* dag, u32 asset_count);

RA_Result RA_dag_parse(RA_DependencyDag* dag, u8* data, u32 size) {
	RA_Result result;
	
	memset(dag, 0, sizeof(RA_DependencyDag));
	RA_arena_create(&dag->arena);
	
	dag->file_data = data;
	dag->file_size = size;
	
	RA_DatFile dat;
	if((result = RA_dat_parse(&dat, data, size, sizeof(RA_DependencyDagFileHeader))) != RA_SUCCESS) {
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
		if(lump->type_crc == LUMP_ASSET_TYPES) {
			if((result = alloc_assets(dag, lump->size)) != RA_SUCCESS) {
				return result;
			}
			for(u32 i = 0; i < dag->asset_count; i++) {
				dag->assets[i].type = lump->data[i];
			}
			has_asset_types_lump = true;
		} else if(lump->type_crc == LUMP_SPACED_OUT_HASH) {
			if((result = alloc_assets(dag, lump->size / 8)) != RA_SUCCESS) {
				return result;
			}
			u64* hashes = (u64*) lump->data;
			for(u32 i = 0; i < dag->asset_count; i++) {
				dag->assets[i].hash = hashes[i];
			}
			has_hashes_lump = true;
		} else if(lump->type_crc == LUMP_DEPENDENCY) {
			dependencies = RA_arena_alloc(&dag->arena, lump->size);
			memcpy(dependencies, lump->data, lump->size);
			dependency_lump_size = lump->size;
			has_dependencies_lump = true;
		} else if(lump->type_crc == LUMP_DAG_PATHS) {
			if((result = alloc_assets(dag, lump->size / 4)) != RA_SUCCESS) {
				return result;
			}
			u32* path_offsets = (u32*) lump->data;
			for(u32 i = 0; i < dag->asset_count; i++) {
				dag->assets[i].path = (char*) (data + sizeof(RA_DependencyDagFileHeader) + path_offsets[i]);
				dag->assets[i].path_crc = RA_crc64_path(dag->assets[i].path);
			}
			has_file_paths_lump = true;
		} else if(lump->type_crc == LUMP_DEPENDENCY_INDEX) {
			if((result = alloc_assets(dag, lump->size / 4)) != RA_SUCCESS) {
				return result;
			}
			s32* dependency_indices = (s32*) lump->data;
			for(u32 i = 0; i < dag->asset_count; i++) {
				s32 dependency_list_index = dependency_indices[i];
				if(dependency_list_index > -1) {
					if(!dependencies) return RA_FAILURE("no dependencies lump");
					if(dependency_list_index * 4 >= dependency_lump_size)
						return RA_FAILURE("dependency list index out of range (%d)", i);
					dag->assets[i].dependencies = (u32*) &dependencies[dependency_list_index];
					s32* dependency = &dependencies[dependency_list_index];
					while(*dependency > -1) {
						if(*dependency >= dag->asset_count) return RA_FAILURE("dependency index out of range");
						dag->assets[i].dependency_count++;
						dependency++;
					}
				}
			}
			has_dependency_index_lump = true;
		}
	}
	
	RA_dat_free(&dat, DONT_FREE_FILE_DATA);
	
	if(!(has_asset_types_lump && has_hashes_lump && has_dependencies_lump && has_file_paths_lump && has_dependency_index_lump)) {
		RA_arena_destroy(&dat.arena);
		return RA_FAILURE("insufficient lumpology");
	}
	
	return RA_SUCCESS;
}

static RA_Result alloc_assets(RA_DependencyDag* dag, u32 asset_count) {
	if(dag->assets == NULL) {
		dag->assets = RA_arena_calloc(&dag->arena, asset_count, sizeof(RA_DependencyDagAsset));
		dag->asset_count = asset_count;
	} else if(dag->asset_count != asset_count) {
		return RA_FAILURE("asset count mismatch");
	}
	return RA_SUCCESS;
}

RA_Result RA_dag_build(RA_DependencyDag* dag, u8** data_dest, u32* size_dest) {
	RA_DatWriter* writer = RA_dat_writer_begin(RA_ASSET_TYPE_DAG, 0xc);
	
	u32 dependency_count = 0;
	for(u32 i = 0; i < dag->asset_count; i++) {
		dependency_count += dag->assets[i].dependency_count + 1;
	}
	
	u64* hashes = RA_dat_writer_lump(writer, LUMP_SPACED_OUT_HASH, dag->asset_count * 8);
	u32* paths = RA_dat_writer_lump(writer, LUMP_DAG_PATHS, dag->asset_count * 4);
	u8* asset_types = RA_dat_writer_lump(writer, LUMP_ASSET_TYPES, dag->asset_count);
	u32* dependency_indices = RA_dat_writer_lump(writer, LUMP_DEPENDENCY_INDEX, dag->asset_count * 4);
	s32* dependency = RA_dat_writer_lump(writer, LUMP_DEPENDENCY, dependency_count * 4);
	u32* unk = RA_dat_writer_lump(writer, LUMP_DAG_UNKNOWN, 1);
	
	RA_dat_writer_string(writer, "DependencyDAG");
	
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

void RA_dag_free(RA_DependencyDag* dag, b8 free_file_data) {
	RA_arena_destroy(&dag->arena);
	if(free_file_data && dag->file_data) {
		free(dag->file_data);
	}
}

RA_DependencyDagAsset* RA_dag_lookup_asset(RA_DependencyDag* dag, u64 path_crc) {
	if(dag->asset_count == 0) {
		return NULL;
	}
	
	u32 first = 0;
	u32 last = dag->asset_count - 1;
	
	while(first <= last) {
		u32 mid = (first + last) / 2;
		RA_DependencyDagAsset* asset = &dag->assets[mid];
		if(asset->path_crc < path_crc) {
			first = mid + 1;
		} else if(asset->path_crc > path_crc) {
			last = mid - 1;
		} else {
			return asset;
		}
	}
	
	return NULL;
}
