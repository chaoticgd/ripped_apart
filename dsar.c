#include "libra/archive.h"

#include <lz4.h>

static void ls(const char* path);
static void extract(const char* input_file, const char* output_dir);
static void print_help();

int main(int argc, char** argv) {
	if(argc < 2) {
		print_help();
	}
	
	if(strcmp(argv[1], "ls") == 0 && argc == 3) {
		ls(argv[2]);
	} else if(strcmp(argv[1], "extract") == 0 && argc == 4) {
		extract(argv[2], argv[3]);
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
	
	printf("Index Offset   Size     Compressed Size\n");
	for(u32 i = 0; i < archive.entry_count; i++) {
		printf("%5u %08" PRIx64 " %08x %08x\n", i, archive.entries[i].offset, archive.entries[i].decompressed_size, archive.entries[i].compressed_size);
	}
	
	RA_archive_close(&archive);
}

static void extract(const char* input_file, const char* output_dir) {
	RA_Result result;
	
	RA_Archive archive;
	if((result = RA_archive_open(&archive, input_file)) != RA_SUCCESS) {
		fprintf(stderr, "Failed to parse archive file '%s' (%s).\n", input_file, result);
		exit(1);
	}
	
	for(u32 i = 0; i < archive.entry_count; i++) {
		u8* compressed;
		u32 compressed_size;
		if((result = RA_archive_read(&archive, i, &compressed, &compressed_size)) != RA_SUCCESS) {
			fprintf(stderr, "Failed to read file %u from archive '%s' (%s).\n", i, input_file, result);
			exit(1);
		}
		
		char path[1024];
		snprintf(path, 1024, "%s/%d.bin", output_dir, i);
		
		FILE* file = fopen(path, "wb");
		if(file == NULL) {
			fprintf(stderr, "Failed to open output file '%s'.\n", path);
			exit(1);
		}
		
		s32 max_decompressed_size = archive.entries[i].decompressed_size;
		u8* decompressed = malloc(max_decompressed_size);
		s32 decompressed_size = LZ4_decompress_safe((char*) compressed, (char*) decompressed, compressed_size, max_decompressed_size);
		if(decompressed_size <= 0) {
			fprintf(stderr, "Failed to decompress file %u from archive '%s' (LZ4_decompress_safe returned %d).\n", i, input_file, decompressed_size);
			exit(1);
		}
		
		if((result = RA_file_write(path, decompressed, decompressed_size)) != RA_SUCCESS) {
			fprintf(stderr, "Failed to write file %d.bin to '%s' (%s).\n", i, path, result);
			exit(1);
		}
	}
	
	RA_archive_close(&archive);
}

static void print_help() {
	puts("Utility for working with DSAR archives, such as those used by the PC version of Rift Apart.");
	puts("");
	puts("Commands:");
	puts("  ls <input file>");
	puts("  extract <input file> <output dir>");
}
