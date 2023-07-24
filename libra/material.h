#ifndef LIBRA_MATERIAL_H
#define LIBRA_MATERIAL_H

#include "util.h"

typedef struct {
	/* 0x00 */ s32 lump_size;
	/* 0x04 */ s32 first_array_count;
	/* 0x08 */ s32 float_array_ofs;
	/* 0x0c */ s32 unknown_c;
	/* 0x10 */ s32 unknown_10;
	/* 0x14 */ s32 texture_count;
	/* 0x18 */ s32 texture_table_ofs;
	/* 0x1c */ s32 texture_strings_ofs;
} SecondMaterialLumpHeader;

typedef struct {
	/* 0x0 */ s32 string_offset;
	/* 0x4 */ u32 unknown_crc;
} TextureTableEntry;

#endif
