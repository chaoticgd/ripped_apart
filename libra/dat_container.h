#ifndef LIBRA_DAT_CONTAINER_H
#define LIBRA_DAT_CONTAINER_H

#include "util.h"
#include "arena.h"

#ifdef __cplusplus
extern "C" {
#endif

// Reader

typedef struct {
	s32 type_crc;
	u32 offset;
	u32 size;
	u8* data;
} RA_DatLump;

typedef struct {
	u8* file_data;
	u32 file_size;
	RA_Arena arena;
	s32 asset_type_crc;
	u32 lump_count;
	RA_DatLump* lumps;
} RA_DatFile;

RA_Result RA_dat_parse(RA_DatFile* dat, u8* data, u32 size, u32 bytes_before_magic); // lumps point into file data
RA_Result RA_dat_read(RA_DatFile* dat, const char* path, u32 bytes_before_magic);    // RA_arena_allocs the lumps
RA_Result RA_dat_free(RA_DatFile* dat, b8 free_file_data);

// Writer

struct t_RA_DatWriter;
typedef struct t_RA_DatWriter RA_DatWriter;

RA_DatWriter* RA_dat_writer_begin(u32 asset_type_crc, u32 bytes_before_magic);   // Begin writing. Call this first.
void* RA_dat_writer_lump(RA_DatWriter* writer, u32 type_crc, s64 size);          // Allocate memory for a lump. Call this second.
u32 RA_dat_writer_string(RA_DatWriter* writer, const char* string);              // Allocate a string. Call this third.
void RA_dat_writer_finish(RA_DatWriter* writer, u8** data_dest, u32* size_dest); // Finish writing, generate the output.
void RA_dat_writer_abort(RA_DatWriter* writer);                                  // Finish writing, don't generate any output.

// Lump type information

typedef enum {
	RA_ASSET_TYPE_DAG = 0x2a077a51,
	RA_ASSET_TYPE_MATERIAL = 0xefe35eab,
	RA_ASSET_TYPE_TEXTURE = 0x8f53a199
} RA_AssetType;

typedef struct {
	b8 has_fake_name;
	const char* name;
	u32 crc;
} RA_LumpType;

void RA_dat_get_lump_types(RA_LumpType** lump_types_dest, s32* lump_type_count_dest);
const char* RA_dat_lump_type_name(u32 type_crc);

// Testing

RA_Result RA_dat_test(const u8* original, u32 original_size, const u8* repacked, u32 repacked_size, b8 print_hex_dump_on_failure);

#ifdef __cplusplus
}
#endif

#endif
