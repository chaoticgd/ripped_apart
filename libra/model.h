#ifndef LIBRA_MODEL_H
#define LIBRA_MODEL_H

#include "util.h"
#include "arena.h"
#include "dat_container.h"

typedef struct {
	const char* name;
} RA_ModelJoint;

typedef struct {
	const char* name;
} RA_ModelLocator;

typedef struct {
	u16 lod_begin;
	u16 lod_count;
} RA_ModelLook;

typedef struct {
	/* 0x00 */ u32 unknown_0;
	/* 0x04 */ u32 unknown_4;
	/* 0x08 */ u32 unknown_8;
	/* 0x0c */ u32 unknown_c;
	/* 0x10 */ u32 unknown_10;
	/* 0x14 */ u32 unknown_14;
	/* 0x18 */ u32 unknown_18;
	/* 0x1c */ u32 unknown_1c;
	/* 0x20 */ u32 unknown_20;
	/* 0x24 */ u32 unknown_24;
	/* 0x28 */ u32 unknown_28;
	/* 0x2c */ f32 scale;
	/* 0x30 */ u32 unknown_30;
	/* 0x34 */ u32 unknown_34;
	/* 0x38 */ u32 unknown_38;
	/* 0x3c */ u32 unknown_3c;
	/* 0x40 */ u32 unknown_40;
	/* 0x44 */ u32 unknown_44;
	/* 0x48 */ u32 unknown_48;
	/* 0x4c */ u32 unknown_4c;
	/* 0x50 */ u32 unknown_50;
	/* 0x54 */ u32 unknown_54;
	/* 0x58 */ u32 unknown_58;
	/* 0x5c */ u32 unknown_5c;
	/* 0x60 */ u32 unknown_60;
	/* 0x64 */ u32 unknown_64;
	/* 0x68 */ u32 unknown_68;
	/* 0x6c */ u32 unknown_6c;
	/* 0x70 */ u32 unknown_70;
	/* 0x74 */ u32 unknown_74;
} RA_ModelBuilt;

typedef struct {
	/* 0x0 */ const char* path;
	/* 0x0 */ const char* name;
} RA_ModelMaterial;

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
	u8* file_data;
	u32 file_size;
	RA_Arena arena;
	u8* physics_data;
	u32 physics_data_size;
	RA_ModelBuilt* built;
	RA_ModelJoint* joints;
	RA_ModelLook* looks;
	RA_ModelMaterial* materials;
	RA_ModelSubset* subsets;
	u16* indices;
	RA_ModelStdVert* std_verts;
	u32 joint_count;
	u32 look_count;
	u32 material_count;
	u32 subset_count;
	u32 index_count;
	u32 std_vert_count;
} RA_Model;

RA_Result RA_model_parse(RA_Model* model, u8* data, u32 size);
void RA_model_unpack_normals(f32* x, f32* y, f32* z, u32 packed);
void RA_model_pack_normals(u32* packed, f32 x, f32 y, f32 z);

#endif
