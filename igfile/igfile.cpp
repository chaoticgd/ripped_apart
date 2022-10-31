#include <stdio.h>
#include <filesystem>

#include <libra/dat_container.h>

namespace fs = std::filesystem;

static void process_file(const char* name);

int main(int argc, char** argv) {
	if(argc != 2) {
		printf("igfile -- https://github.com/chaoticgd/ripped_apart\n");
		printf("usage: %s <path to input asset file>\n", argv[0]);
		printf("       %s <path to input directory>\n", argv[0]);
		return 1;
	}
	
	
}

static void process_file(const char* path) {
	
}
