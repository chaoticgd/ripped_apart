#include "libra/archive.h"

#include <lz4.h>

static void ls(const char* path);
static void decompress(const char* input_path, const char* output_path);
static void print_help();

int main(int argc, char** argv) {
	if(argc == 3 && strcmp(argv[1], "list") == 0) {
		ls(argv[2]);
	} else if(argc == 4 && strcmp(argv[1], "decompress") == 0) {
		decompress(argv[2], argv[3]);
	} else {
		print_help();
		return 1;
	}
}

static void ls(const char* path) {
	RA_Result result;
	
	RA_Archive archive;
	if((result = RA_archive_open(&archive, path)) != RA_SUCCESS) {
		fprintf(stderr, "Failed to load archive file '%s' (%s).\n", path, result->message);
		exit(1);
	}
	
	if(!archive.is_dsar_archive) {
		fprintf(stderr, "error: Input file in not a dsar archive.\n");
		exit(1);
	}
	
	printf("Index Decompressed Offset Compressed Offset   Size     Compressed Size\n");
	for(u32 i = 0; i < archive.dsar_block_count; i++) {
		printf("%5u %19" PRIx64 " %19" PRIx64 " %08x %08x\n",
			i,
			archive.dsar_blocks[i].header.decompressed_offset,
			archive.dsar_blocks[i].header.compressed_offset,
			archive.dsar_blocks[i].header.decompressed_size,
			archive.dsar_blocks[i].header.compressed_size);
	}
	
	RA_archive_close(&archive);
}

static void decompress(const char* input_path, const char* output_path) {
	RA_Result result;
	
	RA_Archive archive;
	if((result = RA_archive_open(&archive, input_path)) != RA_SUCCESS) {
		fprintf(stderr, "Failed to load archive file '%s' (%s).\n", input_path, result->message);
		exit(1);
	}
	
	if(!archive.is_dsar_archive) {
		fprintf(stderr, "error: Input file in not a dsar archive.\n");
		exit(1);
	}
	
	s64 size = RA_archive_get_decompressed_size(&archive);
	if(size == -1) {
		fprintf(stderr, "error: Failed to determine decompressed size.\n");
		exit(1);
	}
	
	u8* data = RA_malloc(size);
	if(data == NULL) {
		fprintf(stderr, "error: Failed to allocate memory for decompressed data.\n");
		exit(1);
	}
	
	if((result = RA_archive_read(&archive, 0, size, data)) != RA_SUCCESS) {
		fprintf(stderr, "error: Failed to decompress data (%s).\n", result->message);
		exit(1);
	}

	if((result = RA_file_write(output_path, data, size)) != RA_SUCCESS) {
		fprintf(stderr, "error: Failed to write output file '%s' (%s).\n", output_path, result->message);
		exit(1);
	}
	
	RA_free(data);
	RA_archive_close(&archive);
}

static void print_help() {
	puts("A utility for working with DSAR archives, such as those used by the PC version of Rift Apart.");
	puts("");
	puts("Commands:");
	puts("  list <input file>");
	puts("  decompress <input file> <output file>");
}
