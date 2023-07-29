#include "dat_container.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	/* 0x0 */ u32 type_crc;
	/* 0x4 */ u32 offset;
	/* 0x8 */ u32 size;
} LumpHeader;

typedef struct {
	/* 0x0 */ char magic[4];
	/* 0x4 */ u32 asset_type_crc;
	/* 0x8 */ u32 file_size;
	/* 0xc */ u16 lump_count;
	/* 0xe */ u16 shader_count;
} DatHeader;

b8 RA_validate_magic_bytes(DatHeader* header) {
	return memcmp(header->magic, "1TAD", 4) == 0;
}

RA_Result RA_dat_parse(RA_DatFile* dat, u8* data, u32 size) {
	memset(dat, 0, sizeof(RA_DatFile));
	RA_arena_create(&dat->arena);
	
	dat->file_data = data;
	dat->file_size = size;
	
	DatHeader* header = (DatHeader*) data;
	if(memcmp(header->magic, "1TAD", 4) != 0) {
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
	LumpHeader* lump_headers = (LumpHeader*) (data + sizeof(DatHeader));
	for(s32 i = 0; i < header->lump_count; i++) {
		dat->lumps[i].type_crc = lump_headers[i].type_crc;
		dat->lumps[i].offset = lump_headers[i].offset;
		dat->lumps[i].size = lump_headers[i].size;
		if(dat->lumps[i].offset + dat->lumps[i].size > size) {
			RA_arena_destroy(&dat->arena);
			return "lump past end of file";
		}
		if(lump_headers[i].size > 256 * 1024 * 1024) {
			RA_arena_destroy(&dat->arena);
			return "lump too big";
		}
		dat->lumps[i].data = data + lump_headers[i].offset;
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

static b8 lump_types_initialised = false;
static RA_LumpType lump_types[] = {
	#define LUMP_TYPE(string, identifier) {string},
	#include "lump_types.h"
	#undef LUMP_TYPE
};
static s32 lump_type_count = ARRAY_SIZE(lump_types);

static void lump_types_init() {
	for(s32 i = 0; i < lump_type_count; i++) {
		const char* name = lump_types[i].name;
		lump_types[i].crc = RA_crc_update((const uint8_t*) lump_types[i].name, strlen(name));
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
