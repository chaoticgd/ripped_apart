#ifndef LIBRA_DAT_CONTAINER_H
#define LIBRA_DAT_CONTAINER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int32_t type_hash;
	int32_t offset;
	int32_t size;
	uint8_t* data;
} RA_DatSection;

typedef struct {
	int32_t asset_type_hash;
	int32_t section_count;
	RA_DatSection* sections;
} RA_DatFile;

RA_DatFile parse_dat_file(const char* path);

#ifdef __cplusplus
}
#endif

#endif
