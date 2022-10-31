#include <stdio.h>
#include <filesystem>

#include <libra/dat_container.h>
#include <libra/texture.h>

namespace fs = std::filesystem;

static RA_Result process_file(const char* name);

int main(int argc, char** argv) {
	if(argc != 2) {
		printf("igfile -- https://github.com/chaoticgd/ripped_apart\n");
		printf("usage: %s <path to input asset file>\n", argv[0]);
		printf("       %s <path to input directory>\n", argv[0]);
		return 1;
	}
	
	if(fs::is_directory(argv[1])) {
		for(const auto& dir_entry : fs::recursive_directory_iterator(argv[1])) {
			process_file(dir_entry.path().string().c_str());
		}
	} else {
		RA_Result result = process_file(argv[1]);
		if(result != NULL) {
			fprintf(stderr, "error: %s\n", result);
		}
	}
}

static RA_Result process_file(const char* path) {
	RA_DatFile dat;
	RA_Result result = RA_parse_dat_file(&dat, path);
	if(result != NULL) {
		return result;
	}
	
	printf("%s ", path);
	
	if(dat.asset_type_hash == ASSET_TYPE_TEXTURE) {
		verify(dat.lump_count > 0 && dat.lumps[0].type_hash == 0x4ede3593, "error: Bad lumps.");
		RA_TextureHeader* tex_header = (RA_TextureHeader*) dat.lumps[0].data;
		const char* format = RA_texture_format_to_string(tex_header->format);
		printf("texture format=%s width=%hd height=%hd", format, tex_header->width, tex_header->height);
	}
	
	printf("\n");
	
	return NULL;
}
