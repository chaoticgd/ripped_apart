#ifndef LIBRA_DAT_CONTAINER_H
#define LIBRA_DAT_CONTAINER_H

#include "util.h"

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
	s32 asset_type_hash;
	s32 lump_count;
	RA_DatLump* lumps;
} RA_DatFile;

typedef enum {
	RA_ASSET_TYPE_TEXTURE = 0x8f53a199
} RA_AssetType;

typedef struct {
	const char* name;
	u32 crc;
} RA_LumpType;

void RA_dat_init();
RA_Result RA_parse_dat_file(RA_DatFile* dat, u8* data, s32 size); // lumps point into data
RA_Result RA_read_dat_file(RA_DatFile* dat, const char* path);    // mallocs the lumps
const char* RA_lump_type_name(u32 type_crc);

extern RA_LumpType dat_lump_types[];
extern s32 dat_lump_type_count;

#ifdef __cplusplus
}
#endif

#endif
