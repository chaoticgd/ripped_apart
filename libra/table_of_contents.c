#include "table_of_contents.h"

#include "dat_container.h"

RA_Result RA_toc_parse(RA_TableOfContents* toc, u8* data, u32 size) {
	RA_Result result;
	
	memset(toc, 0, sizeof(RA_TableOfContents));
	RA_arena_create(&toc->arena);
	
	toc->file_data = data;
	toc->file_size = size;
	
	RA_DatFile dat;
	if((result = RA_dat_parse(&dat, data, size, sizeof(RA_TocFileHeader))) != RA_SUCCESS) {
		RA_arena_destroy(&toc->arena);
		return result;
	}
	
	RA_DatLump* archive_file = RA_dat_lookup_lump(&dat, LUMP_ARCHIVE_FILE);
	if(archive_file == NULL) {
		RA_dat_free(&dat, false);
		RA_arena_destroy(&toc->arena);
		return RA_FAILURE("archive file lump not found");
	}
	
	RA_DatLump* asset_hash = RA_dat_lookup_lump(&dat, LUMP_ASSET_HASH);
	if(asset_hash == NULL) {
		RA_dat_free(&dat, false);
		RA_arena_destroy(&toc->arena);
		return RA_FAILURE("asset hash lump not found");
	}
	
	RA_DatLump* asset_headers = RA_dat_lookup_lump(&dat, LUMP_ASSET_HEADERS);
	if(asset_headers == NULL) {
		RA_dat_free(&dat, false);
		RA_arena_destroy(&toc->arena);
		return RA_FAILURE("asset header lump not found");
	}
	
	RA_DatLump* file_locations = RA_dat_lookup_lump(&dat, LUMP_FILE_LOCATION);
	if(file_locations == NULL) {
		RA_dat_free(&dat, false);
		RA_arena_destroy(&toc->arena);
		return RA_FAILURE("file locations lump not found");
	}
	
	toc->archive_count = archive_file->size / sizeof(RA_TocArchive);
	toc->archives = (RA_TocArchive*) archive_file->data;
	
	toc->asset_count = file_locations->size / sizeof(RA_TocFileLocation);
	toc->assets = RA_arena_alloc(&toc->arena, toc->asset_count * sizeof(RA_TocAsset));
	
	for(u32 i = 0; i < toc->asset_count; i++) {
		toc->assets[i].location = ((RA_TocFileLocation*) file_locations->data)[i];
		toc->assets[i].path_hash = ((u64*) asset_hash->data)[i];
	}
	
	RA_DatLump* asset_groups = RA_dat_lookup_lump(&dat, LUMP_ASSET_GROUPING);
	if(file_locations == NULL) {
		RA_dat_free(&dat, false);
		RA_arena_destroy(&toc->arena);
		return RA_FAILURE("asset groups lump not found");
	}
	
	for(u32 i = 0; i < asset_groups->size / 8; i++) {
		RA_TocAssetGroup* group = &((RA_TocAssetGroup*) asset_groups->data)[i];
		if(group->first_index + group->count > toc->asset_count) {
			RA_dat_free(&dat, false);
			RA_arena_destroy(&toc->arena);
			return RA_FAILURE("asset group out of range");
		}
		for(u32 j = 0; j < group->count; j++) {
			toc->assets[group->first_index + j].group = i;
		}
	}
	
	RA_dat_free(&dat, false);
	return RA_SUCCESS;
}

RA_Result RA_toc_build(RA_TableOfContents* toc, u8** data_dest, u32* size_dest) {
	
	return RA_SUCCESS;
}

void RA_toc_free(RA_TableOfContents* toc, ShouldFreeFileData free_file_data) {
	RA_arena_destroy(&toc->arena);
	if(free_file_data == FREE_FILE_DATA && toc->file_data) {
		free(toc->file_data);
	}
}
