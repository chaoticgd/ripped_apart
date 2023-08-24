#include "libra/archive.h"
#include "libra/dependency_dag.h"
#include "libra/table_of_contents.h"

static void parse_dag_and_toc(RA_DependencyDag* dag, RA_TableOfContents* toc, const char* game_dir);
static void print_help();

static int compare_toc_assets(const void* lhs, const void* rhs) {
	if(((RA_TocAsset*) lhs)->location.archive_index != ((RA_TocAsset*) rhs)->location.archive_index) {
		u32 index_lhs = ((RA_TocAsset*) lhs)->location.archive_index;
		u32 index_rhs = ((RA_TocAsset*) rhs)->location.archive_index;
		if(index_lhs < index_rhs) {
			return -1;
		} else if(index_lhs > index_rhs) {
			return 1;
		} else {
			return 0;
		}
	} else {
		u32 offset_lhs = ((RA_TocAsset*) lhs)->location.offset;
		u32 offset_rhs = ((RA_TocAsset*) rhs)->location.offset;
		if(offset_lhs < offset_rhs) {
			return -1;
		} else if(offset_lhs > offset_rhs) {
			return 1;
		} else {
			return 0;
		}
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
	
	// Sort the assets by archive index, then by file offset. This way we don't
	// have to decompress blocks multiple times.
	qsort(toc.assets, toc.asset_count, sizeof(RA_TocAsset), compare_toc_assets);
	
	RA_Archive archive;
	s32 current_archive_index = -1;
	
	// Extract all the files.
	for(u32 i = 0; i < toc.asset_count; i++) {
		RA_TocAsset* toc_asset = &toc.assets[i];
		RA_DependencyDagAsset* dag_asset = RA_dag_lookup_asset(&dag, toc_asset->path_hash);
		
		// Determine the relative path of the asset.
		char asset_path[RA_MAX_PATH];
		if(dag_asset) {
			strncpy(asset_path, dag_asset->name, RA_MAX_PATH - 1);
		} else {
			// We don't have a proper file path, so just use the asset hash instead.
			if(snprintf(asset_path, RA_MAX_PATH, "%" PRIx64, toc_asset->path_hash) < 0) {
				fprintf(stderr, "error: Output path too long.\n");
				return 1;
			}
		}
		
		// Open the next archive file if necessary.
		if(toc_asset->location.archive_index != current_archive_index) {
			if(current_archive_index != -1) {
				RA_archive_close(&archive);
				current_archive_index = -1;
			}
			if(toc_asset->location.archive_index >= toc.archive_count) {
				fprintf(stderr, "error: Archive index out of range!\n");
				return 1;
			}
			RA_TocArchive* toc_archive = &toc.archives[toc_asset->location.archive_index];
			char archive_path[RA_MAX_PATH];
			if(snprintf(archive_path, RA_MAX_PATH, "%s/%s", game_dir, toc_archive->data) < 0) {
				fprintf(stderr, "error: Output path too long.\n");
				return 1;
			}
			RA_file_fix_path(archive_path + strlen(game_dir));
			if((result = RA_archive_open(&archive, archive_path)) != RA_SUCCESS) {
				fprintf(stderr, "Cannot to open archive '%s'. This is normal for localization files.\n", archive_path);
				continue;
			}
			current_archive_index = toc_asset->location.archive_index;
		}
		
		// Read and decompress blocks as necessary, and assemble the asset.
		u8* data = RA_calloc(1, toc_asset->location.size);
		u32 size = toc_asset->location.size;
		if(data == NULL) {
			fprintf(stderr, "error: Failed allocate memory for asset '%s'.\n", asset_path);
			return 1;
		}
		if((result = RA_archive_read(&archive, toc_asset->location.offset, size, data)) != RA_SUCCESS) {
			fprintf(stderr, "error: Failed to read block for asset '%s' (%s).\n", asset_path, result->message);
			return 1;
		}
		
		// Write out the file.
		char out_path[RA_MAX_PATH];
		if(toc_asset->group > 0) {
			if(snprintf(out_path, RA_MAX_PATH, "%s/%s.%u", out_dir, asset_path, toc_asset->group) < 0) {
				fprintf(stderr, "error: Output path too long.\n");
				return 1;
			}
		} else {
			if(snprintf(out_path, RA_MAX_PATH, "%s/%s", out_dir, asset_path) < 0) {
				fprintf(stderr, "error: Output path too long.\n");
				return 1;
			}
		}
		RA_file_fix_path(out_path + strlen(out_dir));
		if((result = RA_make_dirs(out_path)) != RA_SUCCESS) {
			fprintf(stderr, "error: Failed to make directory for file '%s' (%s).\n", out_path, result->message);
			return 1;
		}
		if((result = RA_file_write(out_path, data, size)) != RA_SUCCESS) {
			fprintf(stderr, "error: Failed to write file '%s' (%s).\n", out_path, result->message);
			return 1;
		}
		
		RA_free(data);
		
		// Write out the list of dependencies for the file.
		if(dag_asset) {
			char deps_out_path[RA_MAX_PATH];
			if(snprintf(deps_out_path, RA_MAX_PATH, "%s/%s.deps", out_dir, asset_path) < 0) {
				fprintf(stderr, "error: Output path too long.\n");
				return 1;
			}
			RA_file_fix_path(deps_out_path + strlen(out_dir));
			FILE* deps_file = fopen(deps_out_path, "w");
			for(u32 j = 0; j < dag_asset->dependency_count; j++) {
				RA_DependencyDagAsset* dependency = &dag.assets[dag_asset->dependencies[j]];
				if(fprintf(deps_file, "%" PRIx64 " %s\n", dependency->name_crc, dependency->name) < 0) {
					fprintf(stderr, "error: Failed to write to dependencies file for asset '%s'.\n", asset_path);
				}
			}
			fclose(deps_file);
		}
	}
}

static void parse_dag_and_toc(RA_DependencyDag* dag, RA_TableOfContents* toc, const char* game_dir) {
	RA_Result result;
	
	// Parse the dependency DAG.
	char dag_path[RA_MAX_PATH];
	snprintf(dag_path, RA_MAX_PATH, "%s/dag", game_dir);
	
	u8* dag_data;
	s64 dag_size;
	if((result = RA_file_read(dag_path, &dag_data, &dag_size))) {
		fprintf(stderr, "error: Failed to read dag file (%s).\n", result->message);
		exit(1);
	}
	
	if((result = RA_dag_parse(dag, dag_data, (u32) dag_size)) != RA_SUCCESS) {
		fprintf(stderr, "error: Failed to parse dag file (%s).\n", result->message);
		exit(1);
	}
	
	// Parse the archive table of contents.
	char toc_path[RA_MAX_PATH];
	snprintf(toc_path, RA_MAX_PATH, "%s/toc", game_dir);
	
	u8* toc_data;
	s64 toc_size;
	if((result = RA_file_read(toc_path, &toc_data, &toc_size)) != RA_SUCCESS) {
		fprintf(stderr, "error: Failed to read toc file (%s).\n", result->message);
		exit(1);
	}
	
	if((result = RA_toc_parse(toc, toc_data, (u32) toc_size)) != RA_SUCCESS) {
		fprintf(stderr, "error: Failed to parse toc file (%s).\n", result->message);
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
