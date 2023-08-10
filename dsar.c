#include "libra/archive.h"

#include <lz4.h>

static void ls(const char* path);
static void print_help();

int main(int argc, char** argv) {
	if(argc == 3 && strcmp(argv[1], "list") == 0) {
		ls(argv[2]);
	} else {
		print_help();
		return 1;
	}
}

static void ls(const char* path) {
	RA_Result result;
	
	RA_Archive archive;
	if((result = RA_archive_open(&archive, path)) != RA_SUCCESS) {
		fprintf(stderr, "Failed to parse archive file '%s' (%s).", path, result);
		exit(1);
	}
	
	printf("Index Decompressed Offset Compressed Offset   Size     Compressed Size\n");
	for(u32 i = 0; i < archive.block_count; i++) {
		printf("%5u %19" PRIx64 " %19" PRIx64 " %08x %08x\n",
			i,
			archive.blocks[i].header.decompressed_offset,
			archive.blocks[i].header.compressed_offset,
			archive.blocks[i].header.decompressed_size,
			archive.blocks[i].header.compressed_size);
	}
	
	RA_archive_close(&archive);
}

static void print_help() {
	puts("A utility for working with DSAR archives, such as those used by the PC version of Rift Apart.");
	puts("");
	puts("Commands:");
	puts("  list <input file>");
}
