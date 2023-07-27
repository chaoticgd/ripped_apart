#ifndef LIBRA_ARCHIVE_H
#define LIBRA_ARCHIVE_H

#include "util.h"

typedef struct {
	/* 0x00 */ u32 magic;
	/* 0x04 */ u32 version;
	/* 0x08 */ u32 file_count;
	/* 0x0c */ u32 data_begin;
	/* 0x10 */ u64 unknown_10;
	/* 0x18 */ char padding[8];
} RA_ArchiveHeader;

typedef struct {
	/* 0x00 */ u64 unknown;
	/* 0x08 */ u64 offset;
	/* 0x10 */ u32 unknown_10;
	/* 0x14 */ u32 size;
	/* 0x18 */ u8 unknown_18;
	/* 0x19 */ u8 unknown_19[7];
} RA_ArchiveEntry;

typedef struct {
	RA_ArchiveEntry* files;
	u32 file_count;
} RA_Archive;

RA_Result RA_archive_read_entries(RA_Archive* archive, const char* path);
RA_Result RA_archive_free(RA_Archive* archive);
RA_Result RA_archive_read_file(RA_Archive* archive, u32 index, u8** data_dest, u32* size_dest);

#endif
