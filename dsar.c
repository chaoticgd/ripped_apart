#include "libra/archive.h"

static void ls(const char* path);
static void print_help();

int main(int argc, char** argv) {
	if(argc < 2) {
		print_help();
	}
	
	if(strcmp(argv[1], "ls") == 0 && argc == 3) {
		ls(argv[2]);
	} else {
		print_help();
		return 1;
	}
}

static void ls(const char* path) {
	RA_Result result;
	
	RA_Archive archive;
	if((result = RA_archive_read_entries(&archive, path)) != RA_SUCCESS) {
		fprintf(stderr, "Failed to parse archive file '%s' (%s).", path, result);
		exit(1);
	}
	
	printf("Index Offset   Size\n");
	for(u32 i = 0; i < archive.file_count; i++) {
		printf("%5u %08" PRIx64 " %08x\n", i, archive.files[i].offset, archive.files[i].size);
	}
}

static void print_help() {
	puts("Commands:");
	puts("  ls <archive file>");
}
