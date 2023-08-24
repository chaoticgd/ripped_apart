#include "dependency_dag.h"

#include "dat_container.h"

RA_Result RA_dag_parse(RA_DependencyDag* dag, u8* data, u32 size) {
	RA_Result result;
	const char* error;
	
	memset(dag, 0, sizeof(RA_DependencyDag));
	RA_arena_create(&dag->arena);
	
	dag->file_data = data;
	dag->file_size = size;
	
	RA_DatFile dat;
	if((result = RA_dat_parse(&dat, data, size, sizeof(RA_DependencyDagFileHeader))) != RA_SUCCESS) {
		return result;
	}
	
	RA_DatLump* asset_ids = RA_dat_lookup_lump(&dat, LUMP_ASSET_IDS);
	if(asset_ids == NULL) {
		error = "no asset ids lump";
		goto fail;
	}
	
	RA_DatLump* asset_names = RA_dat_lookup_lump(&dat, LUMP_ASSET_NAMES);
	if(asset_names == NULL) {
		error = "no asset names lump";
		goto fail;
	}
	
	RA_DatLump* asset_types = RA_dat_lookup_lump(&dat, LUMP_ASSET_TYPES);
	if(asset_types == NULL) {
		error = "no asset types lump";
		goto fail;
	}
	
	RA_DatLump* dependency_index = RA_dat_lookup_lump(&dat, LUMP_DEPENDENCY_INDEX);
	if(dependency_index == NULL) {
		error = "no dependency index lump";
		goto fail;
	}
	
	RA_DatLump* dependency = RA_dat_lookup_lump(&dat, LUMP_DEPENDENCY);
	if(dependency == NULL) {
		error = "no dependency lump";
		goto fail;
	}
	
	u32 asset_count = asset_types->size;
	if(asset_ids->size / 8 != asset_count) {
		error = "asset names or asset ids lump has bad size";
		goto fail;
	}
	if(asset_names->size / 4 != asset_count) {
		error = "asset names or asset ids lump has bad size";
		goto fail;
	}
	if(dependency_index->size / 4 != asset_count) {
		error = "asset names or asset ids lump has bad size";
		goto fail;
	}
	
	dag->assets = RA_arena_calloc(&dag->arena, asset_count, sizeof(RA_DependencyDagAsset));
	dag->asset_count = asset_count;
	if(dag->assets == NULL) {
		error = "cannot allocate asset list";
		goto fail;
	}
	
	for(u32 i = 0; i < asset_count; i++) {
		dag->assets[i].id = ((u64*) asset_ids->data)[i];
		dag->assets[i].name = (char*) (data + sizeof(RA_DependencyDagFileHeader) + ((u32*) asset_names->data)[i]);
		dag->assets[i].name_crc = RA_crc64_path(dag->assets[i].name);
		dag->assets[i].type = asset_types->data[i];
		
		s32 dependency_list_index = ((u32*) dependency_index->data)[i];
		if(dependency_list_index > -1) {
			if(dependency_list_index * 4 >= dependency->size) {
				error = "dependency list index out of range";
				goto fail;
			}
			dag->assets[i].dependencies = (s32*) &((s32*) dependency->data)[dependency_list_index];
			s32* dep = dag->assets[i].dependencies;
			while(*dep > -1) {
				if(*dep >= dag->asset_count) {
					error = "dependency out of range";
					goto fail;
				}
				dag->assets[i].dependency_count++;
				dep++;
			}
		}
	}
	
	RA_dat_free(&dat, DONT_FREE_FILE_DATA);
	return RA_SUCCESS;
	
fail:
	RA_dat_free(&dat, DONT_FREE_FILE_DATA);
	RA_arena_destroy(&dat.arena);
	return RA_FAILURE(error);
}

RA_Result RA_dag_build(RA_DependencyDag* dag, u8** data_dest, s64* size_dest) {
	RA_Result result;
	
	u32 dependency_count = 0;
	for(u32 i = 0; i < dag->asset_count; i++) {
		if(dag->assets[i].dependency_count > 0) {
			dependency_count += dag->assets[i].dependency_count + 1;
		}
	}
	
	RA_DatWriter* writer = RA_dat_writer_begin(RA_ASSET_TYPE_DAG, 0xc);
	if(writer == NULL) {
		return RA_FAILURE("cannot allocate dat writer");
	}
	
	u64* asset_ids = RA_dat_writer_lump(writer, LUMP_ASSET_IDS, dag->asset_count * 8);
	u32* names = RA_dat_writer_lump(writer, LUMP_ASSET_NAMES, dag->asset_count * 4);
	u8* asset_types = RA_dat_writer_lump(writer, LUMP_ASSET_TYPES, dag->asset_count);
	u32* dependency_indices = RA_dat_writer_lump(writer, LUMP_DEPENDENCY_INDEX, dag->asset_count * 4);
	s32* dependency = RA_dat_writer_lump(writer, LUMP_DEPENDENCY, dependency_count * 4);
	u32* unk = RA_dat_writer_lump(writer, LUMP_DAG_UNKNOWN, 1);
	u32 dependency_dag_string_offset = RA_dat_writer_string(writer, "DependencyDAG");
	
	b8 allocation_failed =
		asset_ids == NULL ||
		names == NULL ||
		asset_types == NULL ||
		dependency_indices == NULL ||
		dependency == NULL ||
		unk == NULL ||
		dependency_dag_string_offset == 0;
	if(allocation_failed) {
		RA_dat_writer_abort(writer);
		return RA_FAILURE("cannot allocate lumps");
	}
	
	for(u32 i = 0; i < dag->asset_count; i++) {
		asset_types[i] = dag->assets[i].type;
		asset_ids[i] = dag->assets[i].id;
		names[i] = RA_dat_writer_string(writer, dag->assets[i].name);
		if(dag->assets[i].dependency_count > 0) {
			for(u32 j = 0; j < dag->assets[i].dependency_count; j++) {
				(*dependency++) = dag->assets[i].dependencies[j];
			}
			(*dependency++) = -1;
		}
	}
	
	if((result = RA_dat_writer_finish(writer, data_dest, size_dest)) != RA_SUCCESS) {
		RA_dat_writer_abort(writer);
		return RA_FAILURE(result->message);
	}
	
	return RA_SUCCESS;
}

void RA_dag_free(RA_DependencyDag* dag, b8 free_file_data) {
	RA_arena_destroy(&dag->arena);
	if(free_file_data && dag->file_data) {
		RA_free(dag->file_data);
	}
}

RA_DependencyDagAsset* RA_dag_lookup_asset(RA_DependencyDag* dag, u64 name_crc) {
	if(dag->asset_count == 0) {
		return NULL;
	}
	
	s64 first = 0;
	s64 last = dag->asset_count - 1;
	
	while(first <= last) {
		s64 mid = (first + last) / 2;
		RA_DependencyDagAsset* asset = &dag->assets[mid];
		if(asset->name_crc < name_crc) {
			first = mid + 1;
		} else if(asset->name_crc > name_crc) {
			last = mid - 1;
		} else {
			return asset;
		}
	}
	
	return NULL;
}
