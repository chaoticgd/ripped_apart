#include <stdio.h>
#include <filesystem>

#include <libra/dat_container.h>
#include <libra/texture.h>

namespace fs = std::filesystem;

static RA_Result process_file(const char* name, bool print_lumps);

static enum {
	SORT_CRC,
	SORT_OFFSET,
	SORT_SIZE
} sort_mode;
static s32 header_offset = 0;

int main(int argc, char** argv) {
	if(argc < 2) {
		fprintf(stderr, "igfile -- https://github.com/chaoticgd/ripped_apart\n");
		fprintf(stderr, "usage: %s [-sc|-so|-ss] [-h <header offset>] <input paths>\n", argv[0]);
		return 1;
	}
	
	for(int i = 1; i < argc; i++) {
		if(strcmp(argv[i], "-sc") == 0) {
			sort_mode = SORT_CRC;
			continue;
		}
		
		if(strcmp(argv[i], "-so") == 0) {
			sort_mode = SORT_OFFSET;
			continue;
		}
		
		if(strcmp(argv[i], "-ss") == 0) {
			sort_mode = SORT_SIZE;
			continue;
		}
		
		if(strcmp(argv[i], "-h") == 0) {
			if(i + 1 >= argc) {
				fprintf(stderr, "error: Missing offset argument.\n");
				return 1;
			}
			header_offset = atoi(argv[++i]);
		}
		
		if(fs::is_directory(argv[i])) {
			for(const auto& dir_entry : fs::recursive_directory_iterator(argv[i])) {
				process_file(dir_entry.path().string().c_str(), false);
			}
		} else {
			RA_Result result = process_file(argv[i], true);
			if(result != NULL) {
				fprintf(stderr, "error: Failed to parse DAT1 header (%s).\n", result->message);
			}
		}
	}
}

static int sort_offset(const void* lhs, const void* rhs) { return ((RA_DatLump*) lhs)->offset > ((RA_DatLump*) rhs)->offset; }
static int sort_size(const void* lhs, const void* rhs) { return ((RA_DatLump*) lhs)->size > ((RA_DatLump*) rhs)->size; }

static RA_Result process_file(const char* path, bool print_lumps) {
	RA_Result result;
	
	RA_DatFile dat;
	if((result = RA_dat_read(&dat, path, header_offset)) != RA_SUCCESS) {
		return result;
	}
	
	printf("%s", path);
	
	if(dat.asset_type_crc == RA_ASSET_TYPE_TEXTURE) {
		verify(dat.lump_count > 0 && dat.lumps[0].type_crc == 0x4ede3593, "error: Bad lumps.");
		RA_TextureHeader* tex_header = (RA_TextureHeader*) dat.lumps[0].data;
		const char* format = RA_texture_format_to_string(tex_header->format);
		printf(" texture format=%s width=%hd height=%hd", format, tex_header->width, tex_header->height);
	}
	
	printf("\n");
	
	if(print_lumps) {
		switch(sort_mode) {
			case SORT_CRC: break;
			case SORT_OFFSET: qsort(dat.lumps, dat.lump_count, sizeof(RA_DatLump), sort_offset); break;
			case SORT_SIZE: qsort(dat.lumps, dat.lump_count, sizeof(RA_DatLump), sort_size); break;
		}
		
		printf("lumps:\n\n");
		printf("CRC      | Offset   | Size     | Name\n");
		for(int32_t i = 0; i < dat.lump_count; i++) {
			RA_DatLump* lump = &dat.lumps[i];
			
			printf("%08x | %8x | %8x | %s\n", lump->type_crc, dat.bytes_before_magic + lump->offset, lump->size, RA_dat_lump_type_name(lump->type_crc));
		}
	}
	
	return NULL;
}
