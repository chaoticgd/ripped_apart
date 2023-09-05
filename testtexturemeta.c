#include "libra/archive.h"
#include "libra/texture.h"
#include "libra/dat_container.h"
#include "libra/dependency_dag.h"
#include "libra/table_of_contents.h"

static void parse_dag_and_toc(RA_DependencyDag* dag, RA_TableOfContents* toc, const char* game_dir);
static void print_help();

static int compare_toc_assets(const void* lhs, const void* rhs) {
	if(((RA_TocAsset*) lhs)->metadata.archive_index != ((RA_TocAsset*) rhs)->metadata.archive_index) {
		u32 index_lhs = ((RA_TocAsset*) lhs)->metadata.archive_index;
		u32 index_rhs = ((RA_TocAsset*) rhs)->metadata.archive_index;
		if(index_lhs < index_rhs) {
			return -1;
		} else if(index_lhs > index_rhs) {
			return 1;
		} else {
			return 0;
		}
	} else {
		u32 offset_lhs = ((RA_TocAsset*) lhs)->metadata.offset;
		u32 offset_rhs = ((RA_TocAsset*) rhs)->metadata.offset;
		if(offset_lhs < offset_rhs) {
			return -1;
		} else if(offset_lhs > offset_rhs) {
			return 1;
		} else {
			return 0;
		}
	}
}

static void build_texture_metadata(RA_TocTextureMeta* dest, RA_TextureHeader* src, RA_TocTextureMeta* original) {
	memset(dest, 0, sizeof(RA_TocTextureMeta));
	dest->unknown_0 = original->unknown_0; // TODO
	dest->unknown_8 = 1;
	dest->width_in_texture_file = src->width_in_texture_file;
	dest->height_in_texture_file = src->height_in_texture_file;
	dest->unknown_1c = original->unknown_1c; // TODO
	dest->unknown_20 = original->unknown_20; // TODO
	dest->unknown_22 = 0x2acf;
	dest->format = src->format;
	dest->unknown_28 = 1;
	dest->unknown_30 = 3;
	dest->unknown_34 = original->unknown_34; // TODO
	dest->total_size = src->texture_size + src->streamed_size;
	dest->streamed_size = src->streamed_size;
	dest->unknown_40 = src->unknown_20;
	dest->unknown_41 = (u8) src->unknown_1e;
}

int main(int argc, char** argv) {
	RA_Result result;
	
	if(argc != 2) {
		print_help();
		return 1;
	}
	
	const char* game_dir = argv[1];
	
	RA_DependencyDag dag;
	RA_TableOfContents toc;
	parse_dag_and_toc(&dag, &toc, game_dir);
	
	// Sort the assets by archive index, then by file offset. This way we don't
	// have to decompress blocks multiple times.
	qsort(toc.assets, toc.asset_count, sizeof(RA_TocAsset), compare_toc_assets);
	
	RA_Archive archive;
	s32 current_archive_index = -1;
	
	s32 bailout_counter = 0;
	s32 good_textures = 0;
	
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
		if(toc_asset->metadata.archive_index != current_archive_index) {
			if(current_archive_index != -1) {
				RA_archive_close(&archive);
				current_archive_index = -1;
			}
			if(toc_asset->metadata.archive_index >= toc.archive_count) {
				fprintf(stderr, "error: Archive index out of range!\n");
				return 1;
			}
			RA_TocArchive* toc_archive = &toc.archives[toc_asset->metadata.archive_index];
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
			current_archive_index = toc_asset->metadata.archive_index;
		}
		
		if(!toc_asset->has_texture_meta) {
			continue;
		}
		
		// Read and decompress blocks as necessary, and assemble the asset.
		u8* data = RA_calloc(1, toc_asset->metadata.size);
		u32 size = toc_asset->metadata.size;
		if(toc_asset->has_header) {
			memcpy(data, &toc_asset->header, sizeof(RA_TocAssetHeader));
		}
		if(data == NULL) {
			fprintf(stderr, "error: Failed allocate memory for asset '%s'.\n", asset_path);
			return 1;
		}
		if((result = RA_archive_read(&archive, toc_asset->metadata.offset, toc_asset->metadata.size, data)) != RA_SUCCESS) {
			fprintf(stderr, "error: Failed to read block for asset '%s' (%s).\n", asset_path, result->message);
			return 1;
		}
		
		RA_DatFile dat;
		if((result = RA_dat_parse(&dat, data, size, 0)) != RA_SUCCESS) {
			fprintf(stderr, "error: Failed to read texture asset '%s' (%s).", asset_path, result->message);
			return 1;
		}
		
		RA_DatLump* lump = RA_dat_lookup_lump(&dat, LUMP_TEXTURE_HEADER);
		if(lump == NULL) {
			fprintf(stderr, "error: Failed to find texture header lump in asset '%s'.", asset_path);
			return 1;
		}
		
		RA_TocTextureMeta meta;
		RA_TocAsset* streamed = RA_toc_lookup_asset(toc.assets, toc.asset_count, toc_asset->path_hash, toc_asset->group + 1);
		build_texture_metadata(&meta, (RA_TextureHeader*) lump->data, &toc_asset->texture_meta);
		
		if(RA_diff_buffers((u8*) &toc_asset->texture_meta, sizeof(RA_TocTextureMeta), (u8*) &meta, sizeof(RA_TocTextureMeta), asset_path, true)) {
			printf("decompressed size: %x\n", toc_asset->metadata.size);
			RA_diff_buffers((u8*) &toc_asset->header, sizeof(RA_TocAssetHeader), NULL, 0, "asset header", true);
			RA_diff_buffers(lump->data, sizeof(RA_TextureHeader), NULL, 0, "texture header", true);
			if(bailout_counter++ > 5) {
				printf("good textures: %d\n", good_textures);
				return 1;
			}
		} else {
			good_textures++;
		}
	}
	
	printf("SUCCESS\n");
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
	puts("usage: ./testterxturemeta <game dir>");
}
