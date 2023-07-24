#include "util.h"

RA_Result RA_read_entire_file(u8** data_dest, s32* size_dest, const char* path) {
	FILE* file = fopen(path, "rb");
	if(!file) {
		return "Failed to open file for reading.";
	}
	
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);
	if(size > 1024 * 1024 * 1024) {
		fclose(file);
		return "File too large.";
	}
	*size_dest = (s32) size;
	
	u8* data = malloc(size);
	if(data == NULL) {
		fclose(file);
		return "Failed to allocate memory for file.";
	}
	if(fread(data, size, 1, file) != 1) {
		free(data);
		fclose(file);
		return "Failed to read file.";
	}
	*data_dest = data;
	
	fclose(file);
	
	return RA_SUCCESS;
}

static u32 crc32_table[256];

void RA_crc_init() {
	u32 polynomial = 0xedb88320;
	for (u32 i = 0; i < 256; i++) {
		u32 accumulator = i;
		for(s32 j = 0; j < 8; j++) {
			if (accumulator & 1) {
				accumulator = polynomial ^ (accumulator >> 1);
			} else {
				accumulator >>= 1;
			}
		}
		crc32_table[i] = accumulator;
	}
}

u32 RA_crc_update(const u8* data, s64 size) {
	u32 crc = 0xedb88320;
	for (s64 i = 0; i < size; i++) {
		crc = crc32_table[(crc ^ data[i]) & 0xff] ^ (crc >> 8);
	}
	return crc;
}

u32 RA_crc_string(const char* string) {
	return RA_crc_update((const u8*) string, strlen(string));
}
