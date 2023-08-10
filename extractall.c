#include "libra/archive.h"
#include "libra/dependency_dag.h"
#include "libra/table_of_contents.h"

static void parse_dag_and_toc(RA_DependencyDag* dag, RA_TableOfContents* toc, const char* game_dir);
static void print_help();

static int compare_toc_assets(const void* lhs, const void* rhs) {
	if(((RA_TocAsset*) lhs)->location.archive_index != ((RA_TocAsset*) rhs)->location.archive_index) {
		return ((RA_TocAsset*) lhs)->location.archive_index > ((RA_TocAsset*) rhs)->location.archive_index;
	} else {
		return ((RA_TocAsset*) lhs)->location.decompressed_offset > ((RA_TocAsset*) rhs)->location.decompressed_offset;
	}
}

int main(int argc, char** argv) {
	RA_Result result;
	
	if(argc != 3) {
		print_help();
		return 1;
	}
	
	const char* game_dir = argv[1];
	const char* out_dir = argv[2];
	
	RA_DependencyDag dag;
	RA_TableOfContents toc;
	parse_dag_and_toc(&dag, &toc, game_dir);
	
	if(toc.archive_count > RA_MAX_ARCHIVE_COUNT) {
		fprintf(stderr, "Over %d archive files. Oh no.\n", RA_MAX_ARCHIVE_COUNT);
		return 1;
	}
	
	// Sort the assets by archive index, then by file offset. This way we can
	// open archive files and decompress blocks one by one.
	qsort(toc.assets, toc.asset_count, sizeof(RA_TocAsset), compare_toc_assets);
	
	RA_Archive archive;
	s32 current_archive_index = -1;
	
	// Extract all the files.
	for(u32 i = 0; i < toc.asset_count; i++) {
		RA_TocAsset* toc_asset = &toc.assets[i];
		RA_DependencyDagAsset* dag_asset = RA_dag_lookup_asset(&dag, toc_asset->path_hash);
		if(dag_asset == NULL) {
			continue;
		}
		
		// Open the next archive file if necessary.
		if(toc_asset->location.archive_index != current_archive_index) {
			if(current_archive_index != -1) {
				current_archive_index = -1;
				RA_archive_close(&archive);
			}
			if(toc_asset->location.archive_index >= toc.archive_count) {
				fprintf(stderr, "Archive index out of range!\n");
				continue;
			}
			char archive_path[RA_MAX_PATH];
			snprintf(archive_path, RA_MAX_PATH, "%s/%s", game_dir, toc.archives[toc_asset->location.archive_index].data);
			RA_file_fix_path(archive_path + strlen(game_dir));
			if((result = RA_archive_open(&archive, archive_path)) != RA_SUCCESS) {
				continue;
			}
			current_archive_index = toc_asset->location.archive_index;
		}
		
		// Read and decompress blocks as necessary, and assemble the asset.
		u8* data = calloc(1, toc_asset->location.size);
		u32 size = toc_asset->location.size;
		if((result = RA_archive_read_decompressed(&archive, data, toc_asset->location.decompressed_offset, toc_asset->location.size)) != RA_SUCCESS) {
			fprintf(stderr, "Failed to read block for asset '%s' (%s).", dag_asset->path, result);
			return 1;
		}
		
		// Extract the file.
		char out_path[RA_MAX_PATH];
		snprintf(out_path, RA_MAX_PATH, "%s/%s", out_dir, dag_asset->path);
		RA_file_fix_path(out_path + strlen(out_dir));
		if((result = RA_make_dirs(out_path)) != RA_SUCCESS) {
			fprintf(stderr, "Failed to make directory for file '%s' (%s).\n", out_path, result);
			return 1;
		}
		if((result = RA_file_write(out_path, data, size)) != RA_SUCCESS) {
			fprintf(stderr, "Failed to write file '%s' (%s).\n", out_path, result);
			return 1;
		}
	}
}

static void parse_dag_and_toc(RA_DependencyDag* dag, RA_TableOfContents* toc, const char* game_dir) {
	RA_Result result;
	
	// Parse the dependency DAG.
	char dag_path[RA_MAX_PATH];
	snprintf(dag_path, RA_MAX_PATH, "%s/dag", game_dir);
	
	u8* dag_data;
	u32 dag_size;
	if((result = RA_file_read(&dag_data, &dag_size, dag_path))) {
		fprintf(stderr, "Failed to read dag file (%s).\n", result);
		exit(1);
	}
	
	if((result = RA_dag_parse(dag, dag_data, dag_size)) != RA_SUCCESS) {
		fprintf(stderr, "Failed to parse dag file (%s).\n", result);
		exit(1);
	}
	
	// Parse the archive table of contents.
	char toc_path[RA_MAX_PATH];
	snprintf(toc_path, RA_MAX_PATH, "%s/toc", game_dir);
	
	u8* toc_data;
	u32 toc_size;
	if((result = RA_file_read(&toc_data, &toc_size, toc_path)) != RA_SUCCESS) {
		fprintf(stderr, "Failed to read toc file (%s).\n", result);
		exit(1);
	}
	
	if((result = RA_toc_parse(toc, toc_data, toc_size)) != RA_SUCCESS) {
		fprintf(stderr, "Failed to parse toc file (%s).\n", result);
		exit(1);
	}
}

static void print_help() {
	puts("extractall -- part of https://github.com/chaoticgd/ripped_apart");
	puts("  Extract all assets from all archives into a single tree.");
	puts("");
	puts("Usage:");
	puts("  extractall <game directory> <output directory>");
}
