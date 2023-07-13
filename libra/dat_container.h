#ifndef LIBRA_DAT_CONTAINER_H
#define LIBRA_DAT_CONTAINER_H

#include "util.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int32_t type_crc;
	int32_t offset;
	int32_t size;
	uint8_t* data;
} RA_DatLump;

typedef struct {
	int32_t asset_type_hash;
	int32_t lump_count;
	RA_DatLump* lumps;
} RA_DatFile;

typedef enum {
	RA_ASSET_TYPE_TEXTURE = 0x8f53a199
} RA_AssetType;

typedef struct {
	const char* name;
	uint32_t crc;
} RA_LumpType;

void RA_dat_init();
RA_Result RA_parse_dat_file(RA_DatFile* dat, const char* path);
const char* RA_lump_type_name(uint32_t type_crc);

extern RA_LumpType dat_lump_types[];
extern int32_t dat_lump_type_count;

#ifdef __cplusplus
}
#endif

#endif
