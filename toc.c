#include "libra/dat_container.h"
#include "libra/table_of_contents.h"

static void list_archives(const char* input_file);
static void list_assets(const char* input_file);
static void print_help();

int main(int argc, char** argv) {
	if(argc == 3 && strcmp(argv[1], "list_archives") == 0) {
		list_archives(argv[2]);
	} else if(argc == 3 && strcmp(argv[1], "list_assets") == 0) {
		list_assets(argv[2]);
	} else {
		print_help();
		return 1;
	}
}

static void list_archives(const char* input_file) {
	RA_Result result;
	
	u8* data;
	u32 size;
	if((result = RA_file_read(&data, &size, input_file)) != RA_SUCCESS) {
		fprintf(stderr, "Failed to read input file '%s'.\n", input_file);
		exit(1);
	}
	
	RA_TableOfContents toc;
	if((result = RA_toc_parse(&toc, data, size)) != RA_SUCCESS) {
		fprintf(stderr, "Failed to parse TOC file '%s' (%s).\n", input_file, result->message);
		exit(1);
	}
	
	for(u32 i = 0; i < toc.archive_count; i++) {
		RA_TocArchive* archive = &toc.archives[i];
		printf("%s\n", archive->data);
	}
	
	RA_toc_free(&toc, true);
}

static void list_assets(const char* input_file) {
	RA_Result result;
	
	u8* data;
	u32 size;
	if((result = RA_file_read(&data, &size, input_file)) != RA_SUCCESS) {
		fprintf(stderr, "Failed to read input file '%s'.\n", input_file);
		exit(1);
	}
	
	RA_TableOfContents toc;
	if((result = RA_toc_parse(&toc, data, size)) != RA_SUCCESS) {
		fprintf(stderr, "Failed to parse TOC file '%s' (%s).\n", input_file, result->message);
		exit(1);
	}
	
	printf("Path CRC         Offset   Size     Hdr Ofs  Arch Idx Group\n");
	printf("========         ======   ====     =======  ======== =====\n");
	for(u32 i = 0; i < toc.asset_count; i++) {
		RA_TocAsset* asset = &toc.assets[i];
		printf("%16" PRIx64 " %8x %8x %8x %8x %8x\n",
			asset->path_hash,
			asset->location.offset,
			asset->location.size,
			asset->location.header_offset,
			asset->location.archive_index,
			asset->group);
	}
	
	RA_toc_free(&toc, true);
}

static void print_help() {
	puts("A utility for working with Insomniac Games Dependency TOC files, such as those used by the PC version of Rift Apart.");
	puts("");
	puts("Commands:");
	puts("  list_archives <input file> --- List all the asset archives.");
	puts("  list_assets <input file> --- list all the assets.");
}
