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
	
	if(size < bytes_before_magic + sizeof(DatHeader)) {
		return RA_FAILURE("not enough space for header");
	}
	
	DatHeader* header = (DatHeader*) &data[bytes_before_magic];
	if(header->magic != FOURCC("1TAD")) {
		return RA_FAILURE("bad magic bytes");
	}
	dat->asset_type_crc = header->asset_type_crc;
	dat->lump_count = header->lump_count;
	if(dat->lump_count <= 0) {
		return RA_FAILURE("lump count is zero");
	}
	if(dat->lump_count > 1000) {
		return RA_FAILURE("lump count is too high");
	}
	dat->lumps = RA_arena_alloc(&dat->arena, sizeof(RA_DatLump) * header->lump_count);
	if(dat->lumps == NULL) {
		return RA_FAILURE("allocation failed");
	}
	for(s32 i = 0; i < header->lump_count; i++) {
		dat->lumps[i].type_crc = header->lumps[i].type_crc;
		dat->lumps[i].offset = header->lumps[i].offset;
		dat->lumps[i].size = header->lumps[i].size;
		if(dat->lumps[i].offset + dat->lumps[i].size > size) {
			RA_arena_destroy(&dat->arena);
			return RA_FAILURE("lump past end of file");
		}
		if(header->lumps[i].size > 256 * 1024 * 1024) {
			RA_arena_destroy(&dat->arena);
			return RA_FAILURE("lump too big");
		}
		dat->lumps[i].data = data + bytes_before_magic + header->lumps[i].offset;
	}
	dat->bytes_before_magic = bytes_before_magic;
	return RA_SUCCESS;
}

RA_Result RA_dat_read(RA_DatFile* dat, const char* path, u32 bytes_before_magic) {
	memset(dat, 0, sizeof(RA_DatFile));
	RA_arena_create(&dat->arena);
	
	dat->file_data = NULL;
	dat->file_size = 0;
	
	FILE* file = fopen(path, "rb");
	if(!file) {
		return RA_FAILURE("failed to open file for reading");
	}
	fseek(file, bytes_before_magic, SEEK_SET);
	DatHeader header;
	if(fread(&header, sizeof(DatHeader), 1, file) != 1) {
		fclose(file);
		return RA_FAILURE("failed to read DAT header");
	}
	if(header.magic != FOURCC("1TAD")) {
		fclose(file);
		return RA_FAILURE("bad magic bytes");
	}
	dat->asset_type_crc = header.asset_type_crc;
	dat->lump_count = header.lump_count;
	if(dat->lump_count <= 0) {
		fclose(file);
		return RA_FAILURE("lump count is zero");
	}
	if(dat->lump_count > 1000) {
		fclose(file);
		return RA_FAILURE("lump count is too high");
	}
	dat->lumps = RA_arena_alloc(&dat->arena, sizeof(RA_DatLump) * header.lump_count);
	if(dat->lumps == NULL) {
		return RA_FAILURE("allocation failed");
	}
	LumpHeader* headers = RA_malloc(sizeof(LumpHeader) * header.lump_count);
	if(fread(headers, dat->lump_count * sizeof(LumpHeader), 1, file) != 1) return RA_FAILURE("failed to read lump header");
	for(s32 i = 0; i < header.lump_count; i++) {
		dat->lumps[i].type_crc = headers[i].type_crc;
		dat->lumps[i].offset = headers[i].offset;
		dat->lumps[i].size = headers[i].size;
		if(headers[i].size > 256 * 1024 * 1024) {
			RA_free(headers);
			RA_arena_destroy(&dat->arena);
			fclose(file);
			return RA_FAILURE("lump too big");
		}
		dat->lumps[i].data = RA_arena_alloc(&dat->arena, headers[i].size);
		if(dat->lumps[i].data == NULL) {
			RA_free(headers);
			RA_arena_destroy(&dat->arena);
			fclose(file);
			return RA_FAILURE("allocation failed");
		}
		fseek(file, bytes_before_magic + headers[i].offset, SEEK_SET);
		if(fread(dat->lumps[i].data, headers[i].size, 1, file) != 1) {
			RA_free(headers);
			RA_arena_destroy(&dat->arena);
			fclose(file);
			return RA_FAILURE("failed to read lump");
		}
	}
	dat->bytes_before_magic = bytes_before_magic;
	RA_free(headers);
	fclose(file);
	return RA_SUCCESS;
}

RA_Result RA_dat_free(RA_DatFile* dat, ShouldFreeFileData free_file_data) {
	RA_arena_destroy(&dat->arena);
	if(free_file_data == FREE_FILE_DATA && dat->file_data != NULL) {
		RA_free(dat->file_data);
	}
	return RA_SUCCESS;
}

RA_DatLump* RA_dat_lookup_lump(RA_DatFile* dat, u32 name_crc) {
	if(dat->lump_count == 0) {
		return NULL;
	}
	
	s64 first = 0;
	s64 last = dat->lump_count - 1;
	
	while(first <= last) {
		s64 mid = (first + last) / 2;
		RA_DatLump* lump = &dat->lumps[mid];
		if(lump->type_crc < name_crc) {
			first = mid + 1;
		} else if(lump->type_crc > name_crc) {
			last = mid - 1;
		} else {
			return lump;
		}
	}
	
	return NULL;
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
	RA_DatWriter* writer = RA_calloc(1, sizeof(RA_DatWriter));
	if(writer == NULL) {
		return NULL;
	}
	RA_arena_create(&writer->prologue);
	RA_arena_create(&writer->lumps);
	if(RA_arena_alloc_aligned(&writer->prologue, bytes_before_magic + sizeof(DatHeader), 1) == NULL) {
		return NULL;
	}
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
	if(writer->lumps_size % 0x10 != 0) {
		u32 padding_size = 0x10 - writer->lumps_size % 0x10;
		if(RA_arena_alloc_aligned(&writer->lumps, padding_size, 1) == NULL) {
			return NULL;
		}
		writer->lumps_size += padding_size;
	}
	LumpHeader* header = RA_arena_alloc_aligned(&writer->prologue, sizeof(LumpHeader), 1);
	if(header == NULL) {
		return NULL;
	}
	header->type_crc = type_crc;
	header->offset = writer->lumps_size;
	header->size = size;
	writer->prologue_size += sizeof(LumpHeader);
	writer->lumps_size += size;
	writer->lump_count++;
	writer->write_lump_called = true;
	return RA_arena_alloc(&writer->lumps, size);
}

u32 RA_dat_writer_string(RA_DatWriter* writer, const char* string) {
	u32 string_size = strlen(string) + 1;
	char* allocation = RA_arena_alloc_aligned(&writer->prologue, string_size, 1);
	if(allocation == NULL) {
		return 0;
	}
	strcpy(allocation, string);
	u32 offset = writer->prologue_size;
	writer->prologue_size += string_size;
	writer->write_string_called = true;
	return offset;
}

static int compare_lumps(const void* lhs, const void* rhs) {
	u32 lhs_type = ((LumpHeader*) lhs)->type_crc;
	u32 rhs_type = ((LumpHeader*) rhs)->type_crc;
	if(lhs_type < rhs_type) {
		return -1;
	} else if(lhs_type > rhs_type) {
		return 1;
	} else {
		return 0;
	}
}

RA_Result RA_dat_writer_finish(RA_DatWriter* writer, u8** data_dest, s64* size_dest) {
	if(writer->prologue_size % 0x10 != 0) {
		u32 padding_size = 0x10 - (writer->prologue_size - writer->bytes_before_magic) % 0x10;
		if(RA_arena_alloc_aligned(&writer->prologue, padding_size, 1) == NULL) {
			return RA_FAILURE("cannot allocate padding");
		}
		writer->prologue_size += padding_size;
	}
	*size_dest = writer->prologue_size + writer->lumps_size;
	*data_dest = RA_malloc(*size_dest);
	if(*data_dest == NULL) {
		return RA_FAILURE("cannot allocate output");
	}
	s64 prologue_size = RA_arena_copy(&writer->prologue, *data_dest, *size_dest);
	if(prologue_size != writer->prologue_size) {
		return RA_FAILURE("prologue size mismatch (%u, expected %u)", (u32) prologue_size, writer->prologue_size);
	}
	s64 lumps_size = RA_arena_copy(&writer->lumps, *data_dest + prologue_size, *size_dest - prologue_size);
	if(lumps_size != writer->lumps_size) {
		return RA_FAILURE("lump size mismatch (%u, expected %u)", (u32) lumps_size, writer->lumps_size);
	}
	RA_arena_destroy(&writer->prologue);
	RA_arena_destroy(&writer->lumps);
	DatHeader* header = (DatHeader*) (*data_dest + writer->bytes_before_magic);
	header->magic = FOURCC("1TAD");
	header->asset_type_crc = writer->asset_type_crc;
	header->file_size = *size_dest - writer->bytes_before_magic;
	header->lump_count = writer->lump_count;
	header->shader_count = writer->shader_count;
	for(u32 i = 0; i < header->lump_count; i++) {
		header->lumps[i].offset += writer->prologue_size - writer->bytes_before_magic;
	}
	qsort(header->lumps, header->lump_count, sizeof(LumpHeader), compare_lumps);
	RA_free(writer);
	return RA_SUCCESS;
}

void RA_dat_writer_abort(RA_DatWriter* writer) {
	RA_arena_destroy(&writer->prologue);
	RA_arena_destroy(&writer->lumps);
	RA_free(writer);
}

// Lump type information

RA_LumpType lump_types[] = {
	#define LUMP_TYPE(identifier, crc, name) {name, crc},
	#include "lump_types.h"
	#undef LUMP_TYPE
};
s32 lump_type_count = ARRAY_SIZE(lump_types);

const char* RA_dat_lump_type_name(u32 type_crc) {
	for(s32 i = 0; i < lump_type_count; i++) {
		if(lump_types[i].crc == type_crc) {
			return lump_types[i].name;
		}
	}
	return NULL;
}

// Testing

RA_Result RA_dat_test(const u8* original, u32 original_size, const u8* repacked, u32 repacked_size, b8 print_hex_dump_on_failure) {
	RA_Result result;
	
	if(original_size < sizeof(DatHeader)) return RA_FAILURE("original file only %d bytes in size, too small for dat header", original_size);
	if(repacked_size < sizeof(DatHeader)) return RA_FAILURE("repacked file only %d bytes in size, too small for dat header", repacked_size);
	
	DatHeader* original_header = (DatHeader*) original;
	DatHeader* repacked_header = (DatHeader*) repacked;
	
	if(original_header->magic != FOURCC("1TAD")) return RA_FAILURE("original file is not a dat file");
	if(repacked_header->magic != FOURCC("1TAD")) return RA_FAILURE("repacked file is not a dat file");
	
	if(original_header->asset_type_crc != repacked_header->asset_type_crc)
		return RA_FAILURE("asset types differ, original is %x, repacked is %x", original_header->asset_type_crc, repacked_header->asset_type_crc);
	if(original_header->lump_count != repacked_header->lump_count)
		return RA_FAILURE("lump count is different, original is %hd, repacked is %hd", original_header->lump_count, repacked_header->lump_count);
	
	if(original_size < sizeof(DatHeader) + original_header->lump_count * sizeof(LumpHeader)) return RA_FAILURE("original file too small for lump headers");
	if(repacked_size < sizeof(DatHeader) + repacked_header->lump_count * sizeof(LumpHeader)) return RA_FAILURE("repacked file too small for lump headers");
	
	for(u32 i = 0; i < original_header->lump_count; i++) {
		LumpHeader* original_lump = (LumpHeader*) &original_header->lumps[i];
		LumpHeader* repacked_lump = (LumpHeader*) &repacked_header->lumps[i];
		
		if(original_lump->type_crc != repacked_lump->type_crc) return RA_FAILURE("lump %u crcs differ", i);
		
		if(original_lump->offset + original_lump->size > original_size) return RA_FAILURE("original lump %u out of bounds", i);
		if(repacked_lump->offset + repacked_lump->size > original_size) return RA_FAILURE("repacked lump %u out of bounds", i);
		
		const u8* original_data = &original[original_lump->offset];
		const u8* repacked_data = &repacked[repacked_lump->offset];
		
		RA_LumpType* lump_type = NULL;
		for(u32 j = 0; j < lump_type_count; j++) {
			if(lump_types[j].crc == original_lump->type_crc) {
				lump_type = &lump_types[j];
				break;
			}
		}
		
		char context[256];
		if(lump_type != NULL) {
			snprintf(context, 256, "lump %s", lump_type->name);
		} else {
			snprintf(context, 256, "lump %x", original_lump->type_crc);
		}
		if((result = RA_diff_buffers(original_data, original_lump->size, repacked_data, repacked_lump->size, context, print_hex_dump_on_failure)) != RA_SUCCESS) {
			return result;
		}
	}
	
	// Handle extra padding at the end.
	if(repacked_size < original_size && original_size - repacked_size < 256) {
		original_size = repacked_size;
	}
	
	const char* end_context = "sections match but file data doesn't; issue with strings, lump ordering, DAT header, or alignment";
	if((result = RA_diff_buffers(original, original_size, repacked, repacked_size, end_context, print_hex_dump_on_failure)) != RA_SUCCESS) {
		return result;
	}
	
	return RA_SUCCESS;
}
