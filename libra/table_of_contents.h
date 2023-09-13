#ifndef LIBRA_TABLE_OF_CONTENTS_H
#define LIBRA_TABLE_OF_CONTENTS_H

#include "util.h"
#include "arena.h"

typedef struct {
	/* 0x0 */ u32 unknown_0;
	/* 0x4 */ u32 unknown_4;
} RA_TocFileHeader;

typedef struct {
	/* 0x00 */ char data[0x42];
} RA_TocArchive;

typedef struct {
	/* 0x0 */ u32 first_index;
	/* 0x4 */ u32 count;
} RA_TocAssetGroup;

typedef struct {
	u32 size;
	u32 archive_index;
	u32 offset;
	u32 header_offset;
} RA_TocAssetMetadata;

typedef struct {
	/* 0x00 */ u32 asset_type_hash;
	/* 0x04 */ u32 block_sizes[4];
	/* 0x14 */ u32 unknown_14;
	/* 0x18 */ u32 unknown_18;
	/* 0x1c */ u32 unknown_1c;
	/* 0x20 */ u32 unknown_20;
} RA_TocAssetHeader;

typedef struct {
	/* 0x00 */ u64 unknown_0; // 3 for almost all textures.
	/* 0x08 */ u64 unknown_8;
	/* 0x10 */ u32 width_in_texture_file;
	/* 0x14 */ u32 unknown_14;
	/* 0x18 */ u32 height_in_texture_file;
	/* 0x1c */ u32 unknown_1c; // 1 for almost all textures.
	/* 0x20 */ u16 unknown_20;
	/* 0x22 */ u16 unknown_22; // Same for every texture, seemingly changes between game versions. Uninitialised garbage?
	/* 0x24 */ u32 format;
	/* 0x28 */ u32 unknown_28;
	/* 0x2c */ u32 unknown_2c;
	/* 0x30 */ u32 unknown_30;
	/* 0x34 */ u32 unknown_34; // 0x4000 for almost all textures.
	/* 0x38 */ u32 total_size;
	/* 0x3c */ u32 streamed_size;
	/* 0x40 */ u8 unknown_40;
	/* 0x41 */ u8 unknown_41;
	/* 0x42 */ u8 unknown_42;
	/* 0x43 */ u8 unknown_43;
	/* 0x44 */ u32 unknown_44;
} RA_TocTextureMeta;
RA_ASSERT_SIZE(RA_TocTextureMeta, 0x48);

typedef struct {
	RA_TocAssetMetadata metadata;
	u64 path_hash;
	u32 group;
	b8 has_header;
	RA_TocAssetHeader header;
	b8 has_texture_meta;
	RA_TocTextureMeta texture_meta;
} RA_TocAsset;

typedef struct {
	u8* file_data;
	u32 file_size;
	RA_Arena arena;
	RA_TocFileHeader file_header;
	RA_TocArchive* archives;
	u32 archive_count;
	RA_TocAsset* assets;
	u32 asset_count;
} RA_TableOfContents;

RA_Result RA_toc_parse(RA_TableOfContents* toc, u8* data, u32 size);
RA_Result RA_toc_build(RA_TableOfContents* toc, u8** data_dest, s64* size_dest);
void RA_toc_free(RA_TableOfContents* toc, ShouldFreeFileData free_file_data);
RA_TocAsset* RA_toc_lookup_asset(RA_TocAsset* assets, u32 asset_count, u64 path_hash, u32 group);

#endif
