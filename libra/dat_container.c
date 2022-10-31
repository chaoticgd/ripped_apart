#include "dat_container.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	int32_t type_hash;
	int32_t offset;
	int32_t size;
} LumpHeader;

typedef struct {
	char magic[4];
	int32_t asset_type_hash;
	int32_t file_size;
	int16_t lump_count;
	int16_t shader_count;
} DatHeader;

RA_Result RA_parse_dat_file(RA_DatFile* dat, const char* path) {
	FILE* file = fopen(path, "rb");
	if(!file) {
		return "Failed to open file for reading.";
	}
	DatHeader header;
	if(fread(&header, sizeof(DatHeader), 1, file) != 1) {
		fclose(file);
		return "Failed to read DAT header.";
	}
	if(memcmp(header.magic, "1TAD", 4) != 0) {
		fclose(file);
		return "Bad magic bytes.";
	}
	dat->asset_type_hash = header.asset_type_hash;
	dat->lump_count = header.lump_count;
	if(dat->lump_count <= 0) {
		fclose(file);
		return "Lump count is zero.";
	}
	if(dat->lump_count > 1000) {
		fclose(file);
		return "Lump count is too high!";
	}
	dat->lumps = malloc(sizeof(RA_DatLump) * header.lump_count);
	LumpHeader* headers = malloc(sizeof(LumpHeader) * header.lump_count);
	if(fread(headers, dat->lump_count * sizeof(LumpHeader), 1, file) != 1) return "Failed to read lump header.";
	for(int32_t i = 0; i < header.lump_count; i++) {
		dat->lumps[i].type_hash = headers[i].type_hash;
		dat->lumps[i].offset = headers[i].offset;
		dat->lumps[i].size = headers[i].size;
		if(headers[i].size > 256 * 1024 * 1024) {
			fclose(file);
			return "Lump too big!";
		}
		dat->lumps[i].data = malloc(headers[i].size);
		fseek(file, headers[i].offset, SEEK_SET);
		if(fread(dat->lumps[i].data, headers[i].size, 1, file) != 1) {
			fclose(file);
			return "Failed to read lump.";
		}
	}
	free(headers);
	fclose(file);
	return NULL;
}
