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
} RA_TocFileLocation;

typedef struct {
	/* 0x00 */ u32 asset_type_hash;
	/* 0x04 */ u32 file_size;
	/* 0x08 */ u32 unknown_8;
	/* 0x0c */ u32 unknown_c;
	/* 0x10 */ u32 unknown_10;
	/* 0x14 */ u32 unknown_14;
	/* 0x18 */ u32 unknown_18;
	/* 0x1c */ u32 unknown_1c;
	/* 0x20 */ u32 unknown_20;
} RA_TocAssetHeader;

typedef struct {
	RA_TocFileLocation location;
	u64 path_hash;
	u32 group;
	b8 has_header;
	RA_TocAssetHeader header;
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
	u8* unknown_36;
	u32 unknown_36_size;
	u8* unknown_c9;
	u32 unknown_c9_size;
	u8* unknown_62;
	u32 unknown_62_size;
} RA_TableOfContents;

RA_Result RA_toc_parse(RA_TableOfContents* toc, u8* data, u32 size);
RA_Result RA_toc_build(RA_TableOfContents* toc, u8** data_dest, u32* size_dest);
void RA_toc_free(RA_TableOfContents* toc, ShouldFreeFileData free_file_data);

#endif
