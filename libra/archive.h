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

enum {
	RA_ARCHIVE_COMPRESSION_TEXTURE = 2,
	RA_ARCHIVE_COMPRESSION_LZ4 = 3
};

typedef struct {
	/* 0x00 */ u64 unknown;
	/* 0x08 */ u64 offset;
	/* 0x10 */ u32 decompressed_size;
	/* 0x14 */ u32 compressed_size;
	/* 0x18 */ u8 compression_mode;
	/* 0x19 */ u8 unknown_19[7];
} RA_ArchiveEntry;

typedef struct {
	FILE* file;
	RA_ArchiveEntry* entries;
	u32 entry_count;
} RA_Archive;

RA_Result RA_archive_open(RA_Archive* archive, const char* path);
RA_Result RA_archive_close(RA_Archive* archive);
RA_Result RA_archive_read(RA_Archive* archive, u32 index, u8** data_dest, u32* size_dest);

#endif
