#ifndef LIBRA_DAT_CONTAINER_H
#define LIBRA_DAT_CONTAINER_H

#include <stdint.h>

#include "util.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int32_t type_hash;
	int32_t offset;
	int32_t size;
	uint8_t* data;
} RA_DatLump;

typedef struct {
	int32_t asset_type_hash;
	int32_t lump_count;
	RA_DatLump* lumps;
} RA_DatFile;

RA_Result parse_dat_file(RA_DatFile* dat, const char* path);

#ifdef __cplusplus
}
#endif

#endif
