#include "dat_container.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void RA_dat_init() {
	RA_crc_init();
	for(int32_t i = 0; i < dat_lump_type_count; i++) {
		const char* name = dat_lump_types[i].name;
		dat_lump_types[i].crc = RA_crc_update((const uint8_t*) dat_lump_types[i].name, strlen(name));
	}
}

typedef struct {
	int32_t type_crc;
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
		dat->lumps[i].type_crc = headers[i].type_crc;
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

const char* RA_lump_type_name(uint32_t type_crc) {
	for(int32_t i = 0; i < dat_lump_type_count; i++) {
		if(dat_lump_types[i].crc == type_crc) {
			return dat_lump_types[i].name;
		}
	}
	return NULL;
}

RA_LumpType dat_lump_types[] = {
	{"Texture Header"},
	{"Model Look"},
	{"Model Index"},
	{"Model Built"},
	{"Model Material"},
	{"Model Look Group"},
	{"Model Subset"},
	{"Model Look Built"},
	{"Model Std Vert"},
	{"Model Physics Data"},
	{"Model Joint"},
	{"Model Locator Lookup"},
	{"Model Locator"},
	{"Model Leaf Ids"},
	{"Model Mirror Ids"},
	{"Model Skin Batch"},
	{"Model Skin Data"},
	{"Model Bind Pose"},
	{"Model Joint Lookup"},
	{"Model Col Vert"},
	{"Model Render Overrides"},
	{"Model Splines"},
	{"Model Spline Subsets"},
	{"Model Texture Overrides"},
	{"Zone Scene Objects"},
	{"Zone Impostors Atlas"},
	{"Zone Impostors"},
	{"Zone Asset References"},
	{"Zone Script Actions"},
	{"Zone Script Plugs"},
	{"Zone Script Vars"},
	{"Zone Script Strings"},
	{"Zone Decal Geometry"},
	{"Zone Model Insts"},
	{"Zone Actors"},
	{"Zone Decal Assets"},
	{"Zone Decals"},
	{"Zone Model Names"},
	{"Zone Actor Names"},
	{"Zone Material Overrides"},
	{"Zone Actor Groups"},
	{"Zone Volumes"},
	{"Zone Lights"},
	{"Zone Atmosphere Name"},
	{"Level Link Names"},
	{"Level Zone Names"},
	{"Level Regions Built"},
	{"Level Region Names"},
	{"Level Zones Built"},
	{"Level Built"},
	{"Anim Clip Path"},
	{"Anim Clip Built"},
	{"Anim Clip Curves"},
	{"Conduit Asset Refs"},
	{"Conduit Built"},
	{"Config Type"},
	{"Config Asset Refs"},
	{"Config Built"},
	{"Actor Built"},
	{"Actor Object Built"},
	{"Actor Asset Refs"},
	{"Actor Prius Built"},
	{"Anim Clip Data"},
	{"Anim Clip Lookup"}
};
int32_t dat_lump_type_count = ARRAY_SIZE(dat_lump_types);
