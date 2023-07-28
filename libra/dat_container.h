#ifndef LIBRA_DAT_CONTAINER_H
#define LIBRA_DAT_CONTAINER_H

#include "util.h"
#include "arena.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	s32 type_crc;
	u32 offset;
	u32 size;
	u8* data;
} RA_DatLump;

typedef struct {
	u8* file_data;
	u32 file_size;
	RA_Arena arena;
	s32 asset_type_crc;
	u32 lump_count;
	RA_DatLump* lumps;
} RA_DatFile;

typedef enum {
	RA_ASSET_TYPE_MATERIAL = 0xefe35eab,
	RA_ASSET_TYPE_TEXTURE = 0x8f53a199
} RA_AssetType;

typedef struct {
	const char* name;
	u32 crc;
} RA_LumpType;

void RA_dat_init();
RA_Result RA_dat_parse(RA_DatFile* dat, u8* data, u32 size); // lumps point into file data
RA_Result RA_dat_read(RA_DatFile* dat, const char* path);    // RA_arena_allocs the lumps
RA_Result RA_dat_free(RA_DatFile* dat, b8 free_file_data);
const char* RA_dat_lump_type_name(u32 type_crc);

extern RA_LumpType dat_lump_types[];
extern s32 dat_lump_type_count;

#ifdef __cplusplus
}
#endif

#endif
