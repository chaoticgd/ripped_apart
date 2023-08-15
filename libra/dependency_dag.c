#include "dependency_dag.h"

#include "dat_container.h"

static RA_Result alloc_assets(RA_DependencyDag* dag, u32 asset_count);

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
	
	for(u32 i = 0; i < asset_count; i++) {
		dag->assets[i].id = ((u64*) asset_ids->data)[i];
		dag->assets[i].name = (char*) (data + sizeof(RA_DependencyDagFileHeader) + ((u32*) asset_names->data)[i]);
		dag->assets[i].name_crc = RA_crc64_path(dag->assets[i].name);
		dag->assets[i].type = asset_types->data[i];
		
		s32 dependency_list_index = ((s32*) dependency_index->data)[i];
		if(dependency_list_index > -1) {
			if(dependency_list_index * 4 >= dependency->size)
				return RA_FAILURE("dependency list index out of range (%d)", i);
			dag->assets[i].dependencies = (u32*) &((u32*) dependency->data)[dependency_list_index];
			s32* dep = (s32*) &((s32*) dependency->data)[dependency_list_index];
			while(*dep > -1) {
				if(*dep >= dag->asset_count) return RA_FAILURE("dependency out of range");
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

RA_Result RA_dag_build(RA_DependencyDag* dag, u8** data_dest, u32* size_dest) {
	RA_DatWriter* writer = RA_dat_writer_begin(RA_ASSET_TYPE_DAG, 0xc);
	
	u32 dependency_count = 0;
	for(u32 i = 0; i < dag->asset_count; i++) {
		dependency_count += dag->assets[i].dependency_count + 1;
	}
	
	u64* asset_ids = RA_dat_writer_lump(writer, LUMP_ASSET_IDS, dag->asset_count * 8);
	u32* names = RA_dat_writer_lump(writer, LUMP_ASSET_NAMES, dag->asset_count * 4);
	u8* asset_types = RA_dat_writer_lump(writer, LUMP_ASSET_TYPES, dag->asset_count);
	u32* dependency_indices = RA_dat_writer_lump(writer, LUMP_DEPENDENCY_INDEX, dag->asset_count * 4);
	s32* dependency = RA_dat_writer_lump(writer, LUMP_DEPENDENCY, dependency_count * 4);
	u32* unk = RA_dat_writer_lump(writer, LUMP_DAG_UNKNOWN, 1);
	
	RA_dat_writer_string(writer, "DependencyDAG");
	
	for(u32 i = 0; i < dag->asset_count; i++) {
		asset_types[i] = dag->assets[i].type;
		asset_ids[i] = dag->assets[i].id;
		names[i] = RA_dat_writer_string(writer, dag->assets[i].name);
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

RA_DependencyDagAsset* RA_dag_lookup_asset(RA_DependencyDag* dag, u64 name_crc) {
	if(dag->asset_count == 0) {
		return NULL;
	}
	
	u32 first = 0;
	u32 last = dag->asset_count - 1;
	
	while(first <= last) {
		u32 mid = (first + last) / 2;
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
