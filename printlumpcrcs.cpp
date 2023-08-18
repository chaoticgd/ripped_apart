#include <filesystem>

#include "libra/dat_container.h"

namespace fs = std::filesystem;

int main(int argc, char** argv) {
	RA_Result result;
	
	if(argc != 2) {
		printf("usage: ./printlumpcrcs <asset dir>\n");
		return 1;
	}
	
	for(const auto& dir_entry : fs::recursive_directory_iterator(argv[1])) {
		if(!dir_entry.is_regular_file()) {
			continue;
		}
		
		RA_DatFile dat;
		if((result = RA_dat_read(&dat, dir_entry.path().string().c_str(), 0)) != RA_SUCCESS) {
			continue;
		}
		
		for(u32 i = 0; i < dat.lump_count; i++) {
			printf("%08x\n", dat.lumps[i].type_crc);
		}
		
		RA_dat_free(&dat, FREE_FILE_DATA);
	}
}
