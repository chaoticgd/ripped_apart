#ifndef LIBRA_MATERIAL_H
#define LIBRA_MATERIAL_H

#include "util.h"

typedef struct {
	/* 0x00 */ int32_t lump_size;
	/* 0x04 */ int32_t first_array_count;
	/* 0x08 */ int32_t float_array_ofs;
	/* 0x0c */ int32_t unknown_c;
	/* 0x10 */ int32_t unknown_10;
	/* 0x14 */ int32_t texture_count;
	/* 0x18 */ int32_t texture_table_ofs;
	/* 0x1c */ int32_t texture_strings_ofs;
} SecondMaterialLumpHeader;

typedef struct {
	/* 0x0 */ int32_t string_offset;
	/* 0x4 */ uint32_t unknown_crc;
} TextureTableEntry;

#endif
