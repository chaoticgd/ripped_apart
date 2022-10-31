#include "dat_container.h"

#include <stdio.h>
#include <stdlib.h>

#include "util.h"

typedef struct {
	int32_t type_hash;
	int32_t offset;
	int32_t size;
} SectionHeader;

typedef struct {
	char magic[4];
	int32_t asset_type_hash;
	int32_t file_size;
	int16_t section_count;
	int16_t shader_count;
} DatHeader;

RA_DatFile parse_dat_file(const char* path) {
	RA_DatFile dat;
	FILE* file = fopen(path, "rb");
	if(!file) {
		printf("error: Failed to open '%s' for reading.\n", path);
		exit(1);
	}
	DatHeader header;
	check_fread(fread(&header, sizeof(DatHeader), 1, file));
	dat.asset_type_hash = header.asset_type_hash;
	dat.section_count = header.section_count;
	verify(dat.section_count > 0, "error: No sections!");
	verify(dat.section_count < 1000, "error: Too many sections!");
	dat.sections = malloc(sizeof(RA_DatSection) * header.section_count);
	SectionHeader* headers = malloc(sizeof(SectionHeader) * header.section_count);
	check_fread(fread(headers, dat.section_count * sizeof(SectionHeader), 1, file));
	for(int32_t i = 0; i < header.section_count; i++) {
		dat.sections[i].type_hash = headers[i].type_hash;
		dat.sections[i].offset = headers[i].offset;
		dat.sections[i].size = headers[i].size;
		verify(headers[i].size < 1024 * 1024 * 1024, "error: Section too big!");
		dat.sections[i].data = malloc(headers[i].size);
		fseek(file, headers[i].offset, SEEK_SET);
		check_fread(fread(dat.sections[i].data, headers[i].size, 1, file));
	}
	free(headers);
	fclose(file);
	return dat;
}
