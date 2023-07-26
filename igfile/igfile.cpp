#include <stdio.h>
#include <filesystem>

#include <libra/dat_container.h>
#include <libra/texture.h>

namespace fs = std::filesystem;

static RA_Result process_file(const char* name, bool print_sections);

int main(int argc, char** argv) {
	if(argc < 2) {
		fprintf(stderr, "igfile -- https://github.com/chaoticgd/ripped_apart\n");
		fprintf(stderr, "usage: %s <input paths>\n", argv[0]);
		return 1;
	}
	
	RA_dat_init();
	
	for(int i = 1; i < argc; i++) {
		if(fs::is_directory(argv[i])) {
			for(const auto& dir_entry : fs::recursive_directory_iterator(argv[i])) {
				process_file(dir_entry.path().string().c_str(), false);
			}
		} else {
			RA_Result result = process_file(argv[i], true);
			if(result != NULL) {
				fprintf(stderr, "error: %s\n", result);
			}
		}
	}
}

static RA_Result process_file(const char* path, bool print_sections) {
	RA_DatFile dat;
	RA_Result result = RA_dat_read(&dat, path);
	if(result != NULL) {
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
	
	if(print_sections) {
		printf("sections:\n\n");
		printf("Type     | Offset   | Size\n");
		for(int32_t i = 0; i < dat.lump_count; i++) {
			RA_DatLump* lump = &dat.lumps[i];
			
			printf("%08x | %8x | %8x | %s\n", lump->type_crc, lump->offset, lump->size, RA_dat_lump_type_name(lump->type_crc));
		}
	}
	
	return NULL;
}
