#ifndef LIBRA_MODEL_H
#define LIBRA_MODEL_H

#include "util.h"
#include "dat_container.h"

typedef struct {
	/* 0x00 */ u8 unknown_0[0x14];
	/* 0x14 */ u32 vertex_begin;
	/* 0x18 */ u32 index_begin;
	/* 0x1c */ u32 index_count;
	/* 0x20 */ u16 vertex_count;
	/* 0x22 */ u16 unknown_22;
	/* 0x24 */ u16 flags;
	/* 0x26 */ u16 material;
	/* 0x28 */ u16 bind_table_begin;
	/* 0x2a */ u8 bind_table_count;
	/* 0x2b */ u8 unknown_2b;
	/* 0x2c */ u16 unknown_2c;
	/* 0x2e */ u8 unknown_2e;
	/* 0x2f */ u8 vertex_info_count;
	/* 0x30 */ u8 unknown_31[16];
} RA_ModelSubset;

typedef struct {
	/* 0x0 */ s16 pos_x;
	/* 0x2 */ s16 pos_y;
	/* 0x4 */ s16 pos_z;
	/* 0x6 */ s16 pos_w;
	/* 0x8 */ u32 normals;
	/* 0xc */ s16 texcoord_u;
	/* 0xe */ s16 texcoord_v;
} RA_ModelStdVert;

typedef struct {
	RA_ModelSubset* subsets;
	u16* indices;
	RA_ModelStdVert* std_verts;
	u32 subset_count;
	u32 index_count;
	u32 std_vert_count;
} RA_Model;

RA_Result RA_parse_model(RA_Model* model, RA_DatFile* dat);
void RA_unpack_normals(f32* x, f32* y, f32* z, u32 packed);
void RA_pack_normals(u32* packed, f32 x, f32 y, f32 z);

#endif
