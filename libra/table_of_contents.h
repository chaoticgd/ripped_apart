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
	u32 decompressed_offset;
	u32 header_offset;
} RA_TocFileLocation;

typedef struct {
	RA_TocFileLocation location;
	u64 path_hash;
	u32 group;
} RA_TocAsset;

typedef struct {
	u8* file_data;
	u32 file_size;
	RA_Arena arena;
	RA_TocArchive* archives;
	u32 archive_count;
	RA_TocAsset* assets;
	u32 asset_count;
} RA_TableOfContents;

RA_Result RA_toc_parse(RA_TableOfContents* toc, u8* data, u32 size);
RA_Result RA_toc_build(RA_TableOfContents* toc, u8** data_dest, u32* size_dest);
void RA_toc_free(RA_TableOfContents* toc, ShouldFreeFileData free_file_data);

#endif
