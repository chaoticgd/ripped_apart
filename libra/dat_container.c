#include "dat_container.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Reading

typedef struct {
	/* 0x0 */ u32 type_crc;
	/* 0x4 */ u32 offset;
	/* 0x8 */ u32 size;
} LumpHeader;

typedef struct {
	/* 0x00 */ u32 magic;
	/* 0x04 */ u32 asset_type_crc;
	/* 0x08 */ u32 file_size;
	/* 0x0c */ u16 lump_count;
	/* 0x0e */ u16 shader_count;
	/* 0x10 */ LumpHeader lumps[];
} DatHeader;

RA_Result RA_dat_parse(RA_DatFile* dat, u8* data, u32 size) {
	memset(dat, 0, sizeof(RA_DatFile));
	RA_arena_create(&dat->arena);
	
	dat->file_data = data;
	dat->file_size = size;
	
	DatHeader* header = (DatHeader*) data;
	if(header->magic != FOURCC("1TAD")) {
		return "bad magic bytes";
	}
	dat->asset_type_crc = header->asset_type_crc;
	dat->lump_count = header->lump_count;
	if(dat->lump_count <= 0) {
		return "lump count is zero";
	}
	if(dat->lump_count > 1000) {
		return "lump count is too high";
	}
	dat->lumps = RA_arena_alloc(&dat->arena, sizeof(RA_DatLump) * header->lump_count);
	for(s32 i = 0; i < header->lump_count; i++) {
		dat->lumps[i].type_crc = header->lumps[i].type_crc;
		dat->lumps[i].offset = header->lumps[i].offset;
		dat->lumps[i].size = header->lumps[i].size;
		if(dat->lumps[i].offset + dat->lumps[i].size > size) {
			RA_arena_destroy(&dat->arena);
			return "lump past end of file";
		}
		if(header->lumps[i].size > 256 * 1024 * 1024) {
			RA_arena_destroy(&dat->arena);
			return "lump too big";
		}
		dat->lumps[i].data = data + header->lumps[i].offset;
	}
	return RA_SUCCESS;
}

RA_Result RA_dat_read(RA_DatFile* dat, const char* path) {
	memset(dat, 0, sizeof(RA_DatFile));
	RA_arena_create(&dat->arena);
	
	dat->file_data = NULL;
	dat->file_size = 0;
	
	FILE* file = fopen(path, "rb");
	if(!file) {
		return "failed to open file for reading";
	}
	DatHeader header;
	if(fread(&header, sizeof(DatHeader), 1, file) != 1) {
		fclose(file);
		return "failed to read DAT header";
	}
	if(memcmp(header.magic, "1TAD", 4) != 0) {
		fclose(file);
		return "bad magic bytes";
	}
	dat->asset_type_crc = header.asset_type_crc;
	dat->lump_count = header.lump_count;
	if(dat->lump_count <= 0) {
		fclose(file);
		return "lump count is zero";
	}
	if(dat->lump_count > 1000) {
		fclose(file);
		return "lump count is too high";
	}
	dat->lumps = RA_arena_alloc(&dat->arena, sizeof(RA_DatLump) * header.lump_count);
	LumpHeader* headers = malloc(sizeof(LumpHeader) * header.lump_count);
	if(fread(headers, dat->lump_count * sizeof(LumpHeader), 1, file) != 1) return "failed to read lump header";
	for(s32 i = 0; i < header.lump_count; i++) {
		dat->lumps[i].type_crc = headers[i].type_crc;
		dat->lumps[i].offset = headers[i].offset;
		dat->lumps[i].size = headers[i].size;
		if(headers[i].size > 256 * 1024 * 1024) {
			for(s32 j = 0; j < i; j++) {
				free(dat->lumps[i].data);
			}
			free(headers);
			RA_arena_destroy(&dat->arena);
			fclose(file);
			return "lump too big";
		}
		dat->lumps[i].data = RA_arena_alloc(&dat->arena, headers[i].size);
		fseek(file, headers[i].offset, SEEK_SET);
		if(fread(dat->lumps[i].data, headers[i].size, 1, file) != 1) {
			for(s32 j = 0; j < i; j++) {
				free(dat->lumps[i].data);
			}
			free(headers);
			RA_arena_destroy(&dat->arena);
			fclose(file);
			return "failed to read lump";
		}
	}
	free(headers);
	fclose(file);
	return RA_SUCCESS;
}

RA_Result RA_dat_free(RA_DatFile* dat, b8 free_file_data) {
	RA_arena_destroy(&dat->arena);
	if(free_file_data && dat->file_data != NULL) {
		free(dat->file_data);
	}
	return RA_SUCCESS;
}

// Writing

struct t_RA_DatWriter {
	RA_Arena prologue;
	RA_Arena lumps;
	u32 prologue_size;
	u32 lumps_size;
	u32 asset_type_crc;
	u16 lump_count;
	u16 shader_count;
	b8 write_lump_called;
	b8 write_string_called;
};

RA_DatWriter* RA_dat_writer_begin(u32 asset_type_crc) {
	RA_DatWriter* writer = calloc(1, sizeof(RA_DatWriter));
	RA_arena_create(&writer->prologue);
	RA_arena_create(&writer->lumps);
	RA_arena_alloc_aligned(&writer->prologue, sizeof(DatHeader), 1);
	writer->prologue_size = sizeof(DatHeader);
	writer->asset_type_crc = asset_type_crc;
	return writer;
}

void* RA_dat_writer_lump(RA_DatWriter* writer, u32 type_crc, s64 size) {
	if(writer->write_string_called) {
		fprintf(stderr, "RA_dat_writer_lump: Called after RA_dat_writer_string!\n");
		abort();
	}
	LumpHeader* header = RA_arena_alloc_aligned(&writer->prologue, sizeof(LumpHeader), 1);
	header->offset = writer->lumps_size;
	header->size = size;
	s64 aligned_size = ALIGN(size, 0x10);
	writer->prologue_size += sizeof(LumpHeader);
	writer->lumps_size += aligned_size;
	writer->lump_count++;
	writer->write_lump_called = true;
	return RA_arena_alloc(&writer->lumps, aligned_size);
}

u32 RA_dat_writer_string(RA_DatWriter* writer, const char* string) {
	u32 string_size = strlen(string) + 1;
	char* allocation = RA_arena_alloc_aligned(&writer->prologue, string_size, 1);
	strcpy(allocation, string);
	u32 offset = writer->prologue_size;
	writer->prologue_size += string_size;
	writer->write_string_called = true;
	return offset;
}

void RA_dat_writer_finish(RA_DatWriter* writer, u8** data_dest, u32* size_dest) {
	*size_dest = writer->prologue_size + writer->lumps_size;
	*data_dest = malloc(*size_dest);
	s64 prologue_size = RA_arena_copy(&writer->prologue, *data_dest, *size_dest);
	if(prologue_size != writer->prologue_size) {
		fprintf(stderr, "RA_dat_writer_finish: Prologue size mismatch (%d, expected %d)!\n", (u32) prologue_size, writer->prologue_size);
		abort();
	}
	s64 lumps_size = RA_arena_copy(&writer->lumps, *data_dest + prologue_size, *size_dest - prologue_size);
	if(lumps_size != writer->lumps_size) {
		fprintf(stderr, "RA_dat_writer_finish: Lump size mismatch (%d, expected %d)!\n", (u32) lumps_size, writer->lumps_size);
		abort();
	}
	RA_arena_destroy(&writer->prologue);
	RA_arena_destroy(&writer->lumps);
	DatHeader* header = (DatHeader*) *data_dest;
	header->magic = FOURCC("1TAD");
	header->asset_type_crc = writer->asset_type_crc;
	header->file_size = *size_dest;
	header->lump_count = writer->lump_count;
	header->shader_count = writer->shader_count;
	free(writer);
}

void RA_dat_writer_abort(RA_DatWriter* writer) {
	RA_arena_destroy(&writer->prologue);
	RA_arena_destroy(&writer->lumps);
	free(writer);
}

// Lump type information

static b8 lump_types_initialised = false;
static RA_LumpType lump_types[] = {
	#define LUMP_TYPE(string, identifier) {false, string},
	#define LUMP_TYPE_FAKE_NAME(crc, identifier) {true, #identifier, crc},
	#include "lump_types.h"
	#undef LUMP_TYPE
};
static s32 lump_type_count = ARRAY_SIZE(lump_types);

static void lump_types_init() {
	for(s32 i = 0; i < lump_type_count; i++) {
		if(!lump_types[i].has_fake_name) {
			const char* name = lump_types[i].name;
			lump_types[i].crc = RA_crc_update((const uint8_t*) lump_types[i].name, strlen(name));
		}
	}
	lump_types_initialised = true;
}

void RA_dat_get_lump_types(RA_LumpType** lump_types_dest, s32* lump_type_count_dest) {
	if(!lump_types_initialised) {
		lump_types_init();
	}
	*lump_types_dest = lump_types;
	*lump_type_count_dest = lump_type_count;
}

const char* RA_dat_lump_type_name(u32 type_crc) {
	RA_LumpType* types;
	s32 count;
	RA_dat_get_lump_types(&types, &count);
	for(s32 i = 0; i < count; i++) {
		if(types[i].crc == type_crc) {
			return types[i].name;
		}
	}
	return NULL;
}
