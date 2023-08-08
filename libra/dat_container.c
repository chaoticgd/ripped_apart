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

RA_Result RA_dat_parse(RA_DatFile* dat, u8* data, u32 size, u32 bytes_before_magic) {
	memset(dat, 0, sizeof(RA_DatFile));
	RA_arena_create(&dat->arena);
	
	dat->file_data = data;
	dat->file_size = size;
	
	DatHeader* header = (DatHeader*) &data[bytes_before_magic];
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
		dat->lumps[i].data = data + bytes_before_magic + header->lumps[i].offset;
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
	if(header.magic != FOURCC("1TAD")) {
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
	u32 bytes_before_magic;
	u32 prologue_size;
	u32 lumps_size;
	u32 asset_type_crc;
	u16 lump_count;
	u16 shader_count;
	b8 write_lump_called;
	b8 write_string_called;
};

RA_DatWriter* RA_dat_writer_begin(u32 asset_type_crc, u32 bytes_before_magic) {
	RA_DatWriter* writer = calloc(1, sizeof(RA_DatWriter));
	RA_arena_create(&writer->prologue);
	RA_arena_create(&writer->lumps);
	RA_arena_alloc_aligned(&writer->prologue, bytes_before_magic + sizeof(DatHeader), 1);
	writer->bytes_before_magic = bytes_before_magic;
	writer->prologue_size = bytes_before_magic + sizeof(DatHeader);
	writer->asset_type_crc = asset_type_crc;
	return writer;
}

void* RA_dat_writer_lump(RA_DatWriter* writer, u32 type_crc, s64 size) {
	if(writer->write_string_called) {
		fprintf(stderr, "RA_dat_writer_lump: Called after RA_dat_writer_string!\n");
		abort();
	}
	LumpHeader* header = RA_arena_alloc_aligned(&writer->prologue, sizeof(LumpHeader), 1);
	header->type_crc = type_crc;
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

static int compare_lumps(const void* lhs, const void* rhs) {
	return ((LumpHeader*) lhs)->type_crc > ((LumpHeader*) rhs)->type_crc;
}

void RA_dat_writer_finish(RA_DatWriter* writer, u8** data_dest, u32* size_dest) {
	if(writer->prologue_size % 0x10 != 0) {
		u32 padding_size = 0x10 - writer->prologue_size % 0x10;
		RA_arena_alloc_aligned(&writer->prologue, padding_size, 1);
		writer->prologue_size += padding_size;
	}
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
	DatHeader* header = (DatHeader*) (*data_dest + writer->bytes_before_magic);
	header->magic = FOURCC("1TAD");
	header->asset_type_crc = writer->asset_type_crc;
	header->file_size = *size_dest;
	header->lump_count = writer->lump_count;
	header->shader_count = writer->shader_count;
	for(u32 i = 0; i < header->lump_count; i++) {
		header->lumps[i].offset += writer->prologue_size - writer->bytes_before_magic;
	}
	qsort(header->lumps, header->lump_count, sizeof(LumpHeader), compare_lumps);
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

// Testing

static RA_Result diff_buffers(const u8* lhs, u32 lhs_size, const u8* rhs, u32 rhs_size, u32 lump, b8 print_hex_dump_on_failure);

RA_Result RA_dat_test(const u8* original, u32 original_size, const u8* repacked, u32 repacked_size, b8 print_hex_dump_on_failure) {
	RA_Result result;
	
	if(original_size < sizeof(DatHeader)) return RA_failure("original file only %d bytes in size, too small for dat header", original_size);
	if(repacked_size < sizeof(DatHeader)) return RA_failure("repacked file only %d bytes in size, too small for dat header", repacked_size);
	
	DatHeader* original_header = (DatHeader*) original;
	DatHeader* repacked_header = (DatHeader*) repacked;
	
	if(original_header->magic != FOURCC("1TAD")) return "original file is not a dat file";
	if(repacked_header->magic != FOURCC("1TAD")) return "repacked file is not a dat file";
	
	if(original_header->asset_type_crc != repacked_header->asset_type_crc)
		return RA_failure("asset types differ, original is %x, repacked is %x", original_header->asset_type_crc, repacked_header->asset_type_crc);
	if(original_header->lump_count != repacked_header->lump_count)
		return RA_failure("lump count is different, original is %hd, repacked is %hd", original_header->lump_count, repacked_header->lump_count);
	
	if(original_size < sizeof(DatHeader) + original_header->lump_count * sizeof(LumpHeader)) return "original file too small for lump headers";
	if(repacked_size < sizeof(DatHeader) + repacked_header->lump_count * sizeof(LumpHeader)) return "repacked file too small for lump headers";
	
	for(u32 i = 0; i < original_header->lump_count; i++) {
		LumpHeader* original_lump = (LumpHeader*) &original_header->lumps[i];
		LumpHeader* repacked_lump = (LumpHeader*) &repacked_header->lumps[i];
		
		if(original_lump->type_crc != repacked_lump->type_crc) return RA_failure("lump %u crcs differ", i);
		if(original_lump->size != repacked_lump->size) return RA_failure("lump %u sizes differ", i);
		
		if(original_lump->offset + original_lump->size > original_size) return RA_failure("original lump %u out of bounds", i);
		if(repacked_lump->offset + repacked_lump->size > original_size) return RA_failure("repacked lump %u out of bounds", i);
		
		const u8* original_data = &original[original_lump->offset];
		const u8* repacked_data = &repacked[repacked_lump->offset];
		
		if((result = diff_buffers(original_data, original_lump->size, repacked_data, repacked_lump->size, i, print_hex_dump_on_failure)) != RA_SUCCESS) {
			return result;
		}
	}
	
	return RA_SUCCESS;
}

static RA_Result diff_buffers(const u8* lhs, u32 lhs_size, const u8* rhs, u32 rhs_size, u32 lump, b8 print_hex_dump_on_failure) {
	u32 min_size = MIN(lhs_size, rhs_size);
	u32 max_size = MAX(lhs_size, rhs_size);
	
	u32 diff_offset = UINT32_MAX;
	for(u32 j = 0; j < min_size; j++) {
		if(lhs[j] != rhs[j]) {
			diff_offset = j;
			break;
		}
	}
	
	if(lhs_size == rhs_size && diff_offset == UINT32_MAX) {
		return RA_SUCCESS;
	}
	
	RA_Result error = RA_failure("lump %u differs at offset %x", lump, diff_offset);
	if(!print_hex_dump_on_failure) {
		return error;
	}
	
	printf("%s\n", error);
	
	s64 row_start = (diff_offset / 0x10) * 0x10;
	s64 hexdump_begin = MAX(0, row_start - 0x50);
	s64 hexdump_end = max_size;
	for(s64 i = hexdump_begin; i < hexdump_end; i += 0x10) {
		printf("%08x: ", (s32) i);
		const u8* current = lhs;
		u32 current_size = lhs_size;
		for(u32 j = 0; j < 2; j++) {
			for(s64 j = 0; j < 0x10; j++) {
				s64 pos = i + j;
				const char* colour = NULL;
				if(pos < lhs_size && pos < rhs_size) {
					if(lhs[pos] == rhs[pos]) {
						colour = "32";
					} else {
						colour = "31";
					}
				} else if(pos < current_size) {
					colour = "33";
				} else {
					printf("   ");
				}
				if(colour != NULL) {
					printf("\033[%sm%02x\033[0m ", colour, current[pos]);
				}
				if(j % 4 == 3 && j != 0xf) {
					printf(" ");
				}
			}
			printf("| ");
			current = rhs;
			current_size = rhs_size;
		}
		printf("\n");
	}
	printf("\n");
	
	return error;
}
