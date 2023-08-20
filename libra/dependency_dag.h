#ifndef LIBRA_DEPENDENCY_DAG_H
#define LIBRA_DEPENDENCY_DAG_H

#include "util.h"
#include "arena.h"

typedef struct {
	/* 0x0 */ u32 unknown_0;
	/* 0x4 */ u32 unknown_4;
	/* 0x8 */ u32 unknown_8;
} RA_DependencyDagFileHeader;

typedef struct {
	u64 id;
	const char* name;
	u64 name_crc;
	u8 type;
	s32* dependencies;
	u32 dependency_count;
} RA_DependencyDagAsset;

typedef struct {
	u8* file_data;
	u32 file_size;
	RA_Arena arena;
	RA_DependencyDagFileHeader* header;
	RA_DependencyDagAsset* assets;
	u32 asset_count;
} RA_DependencyDag;

RA_Result RA_dag_parse(RA_DependencyDag* dag, u8* data, u32 size);
RA_Result RA_dag_build(RA_DependencyDag* dag, u8** data_dest, s64* size_dest);
void RA_dag_free(RA_DependencyDag* dag, b8 free_file_data);

RA_DependencyDagAsset* RA_dag_lookup_asset(RA_DependencyDag* dag, u64 name_crc);

#endif
