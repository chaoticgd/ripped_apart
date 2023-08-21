#include "table_of_contents.h"

#include "dat_container.h"

RA_Result RA_toc_parse(RA_TableOfContents* toc, u8* data, u32 size) {
	RA_Result result;
	
	memset(toc, 0, sizeof(RA_TableOfContents));
	RA_arena_create(&toc->arena);
	
	toc->file_data = data;
	toc->file_size = size;
	
	if(size < sizeof(RA_TocFileHeader)) {
		return RA_FAILURE("no space for TOC file header");
	}
	memcpy(&toc->file_header, toc->file_data, sizeof(RA_TocFileHeader));
	
	RA_DatFile dat;
	if((result = RA_dat_parse(&dat, data, size, sizeof(RA_TocFileHeader))) != RA_SUCCESS) {
		RA_arena_destroy(&toc->arena);
		return result;
	}
	
	RA_DatLump* asset_hash = RA_dat_lookup_lump(&dat, LUMP_ASSET_HASH);
	if(asset_hash == NULL) {
		RA_dat_free(&dat, DONT_FREE_FILE_DATA);
		RA_arena_destroy(&toc->arena);
		return RA_FAILURE("asset hash lump not found");
	}
	
	RA_DatLump* archive_file = RA_dat_lookup_lump(&dat, LUMP_ARCHIVE_FILE);
	if(archive_file == NULL) {
		RA_dat_free(&dat, DONT_FREE_FILE_DATA);
		RA_arena_destroy(&toc->arena);
		return RA_FAILURE("archive file lump not found");
	}
	
	RA_DatLump* file_locations = RA_dat_lookup_lump(&dat, LUMP_FILE_LOCATION);
	if(file_locations == NULL) {
		RA_dat_free(&dat, DONT_FREE_FILE_DATA);
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
		RA_dat_free(&dat, DONT_FREE_FILE_DATA);
		RA_arena_destroy(&toc->arena);
		return RA_FAILURE("asset groups lump not found");
	}
	
	for(u32 i = 0; i < asset_groups->size / 8; i++) {
		RA_TocAssetGroup* group = &((RA_TocAssetGroup*) asset_groups->data)[i];
		if(group->first_index + group->count > toc->asset_count) {
			RA_dat_free(&dat, DONT_FREE_FILE_DATA);
			RA_arena_destroy(&toc->arena);
			return RA_FAILURE("asset group out of range");
		}
		for(u32 j = 0; j < group->count; j++) {
			toc->assets[group->first_index + j].group = i;
		}
	}
	
	RA_DatLump* unk_36 = RA_dat_lookup_lump(&dat, LUMP_TOC_UNKNOWN_36);
	if(unk_36 == NULL) {
		RA_dat_free(&dat, DONT_FREE_FILE_DATA);
		RA_arena_destroy(&toc->arena);
		return RA_FAILURE("unk 36 lump not found");
	}
	toc->unknown_36 = unk_36->data;
	toc->unknown_36_size = unk_36->size;
	
	RA_DatLump* unk_c9 = RA_dat_lookup_lump(&dat, LUMP_TOC_UNKNOWN_C9);
	if(unk_c9 == NULL) {
		RA_dat_free(&dat, DONT_FREE_FILE_DATA);
		RA_arena_destroy(&toc->arena);
		return RA_FAILURE("unk c9 lump not found");
	}
	toc->unknown_c9 = unk_c9->data;
	toc->unknown_c9_size = unk_c9->size;
	
	RA_DatLump* unk_62 = RA_dat_lookup_lump(&dat, LUMP_TOC_UNKNOWN_62);
	if(unk_62 == NULL) {
		RA_dat_free(&dat, DONT_FREE_FILE_DATA);
		RA_arena_destroy(&toc->arena);
		return RA_FAILURE("unk 62 lump not found");
	}
	toc->unknown_62 = unk_62->data;
	toc->unknown_62_size = unk_62->size;
	
	RA_DatLump* asset_headers = RA_dat_lookup_lump(&dat, LUMP_ASSET_HEADERS);
	if(asset_headers == NULL) {
		RA_dat_free(&dat, DONT_FREE_FILE_DATA);
		RA_arena_destroy(&toc->arena);
		return RA_FAILURE("asset header lump not found");
	}
	for(u32 i = 0; i < toc->asset_count; i++) {
		u32 header_ofs = toc->assets[i].location.header_offset;
		if(header_ofs != 0xffffffff) {
			toc->assets[i].has_header = true;
			memcpy(&toc->assets[i].header, asset_headers->data + toc->assets[i].location.header_offset, sizeof(RA_TocAssetHeader));
		}
	}
	
	RA_dat_free(&dat, DONT_FREE_FILE_DATA);
	return RA_SUCCESS;
}

static int compare_toc_assets(const void* lhs, const void* rhs) {
	if(((RA_TocAsset*) lhs)->group != ((RA_TocAsset*) rhs)->group) {
		return ((RA_TocAsset*) lhs)->group - ((RA_TocAsset*) rhs)->group;
	} else {
		return ((RA_TocAsset*) lhs)->path_hash - ((RA_TocAsset*) rhs)->path_hash;
	}
}

RA_Result RA_toc_build(RA_TableOfContents* toc, u8** data_dest, s64* size_dest) {
	qsort(toc->assets, toc->asset_count, sizeof(RA_TocAsset), compare_toc_assets);
	
	RA_DatWriter* writer = RA_dat_writer_begin(RA_ASSET_TYPE_TOC, 0x8);
	
	u32 group_count = 256;
	for(u32 i = 0; i < toc->asset_count; i++) {
		group_count = MAX(toc->assets[i].group + 1, group_count);
	}
	RA_TocAssetGroup* asset_groups = RA_dat_writer_lump(writer, LUMP_ASSET_GROUPING, group_count * sizeof(RA_TocAssetGroup));
	memset(asset_groups, 0, group_count * sizeof(RA_TocAssetGroup));
	u32 last_group = 0;
	u32 group_begin = 0;
	u32 asset_group_top = 0;
	for(u32 i = 0; i <= toc->asset_count; i++) {
		if(i == toc->asset_count || toc->assets[i].group != last_group) {
			while(asset_group_top != last_group) {
				if(asset_group_top > last_group) {
					return RA_FAILURE("asset group index mismatch");
				}
				asset_groups[asset_group_top].first_index = group_begin;
				asset_groups[asset_group_top].count = 0;
				asset_group_top++;
			}
			
			asset_groups[asset_group_top].first_index = group_begin;
			asset_groups[asset_group_top].count = i - group_begin;
			asset_group_top++;
			
			if(i < toc->asset_count) {
				last_group = toc->assets[i].group;
			}
			group_begin = i;
		}
	}
	while(asset_group_top < group_count) {
		asset_groups[asset_group_top].first_index = group_begin;
		asset_groups[asset_group_top].count = 0;
		asset_group_top++;
	}
	
	u64* asset_hashes = RA_dat_writer_lump(writer, LUMP_ASSET_HASH, toc->asset_count * sizeof(u64));
	for(u32 i = 0; i < toc->asset_count; i++) {
		asset_hashes[i] = toc->assets[i].path_hash;
	}
	
	RA_TocFileLocation* file_location = RA_dat_writer_lump(writer, LUMP_FILE_LOCATION, toc->asset_count * sizeof(RA_TocFileLocation));
	u32 next_header_offset = 0;
	for(u32 i = 0; i < toc->asset_count; i++) {
		file_location[i] = toc->assets[i].location;
		if(toc->assets[i].has_header) {
			file_location[i].header_offset = next_header_offset;
			next_header_offset += sizeof(RA_TocAssetHeader);
		}
	}
	
	RA_TocArchive* archives = RA_dat_writer_lump(writer, LUMP_ARCHIVE_FILE, toc->archive_count * sizeof(RA_TocArchive));
	memcpy(archives, toc->archives, toc->archive_count * sizeof(RA_TocArchive));
	
	u8* unk_36 = RA_dat_writer_lump(writer, LUMP_TOC_UNKNOWN_36, toc->unknown_36_size);
	memcpy(unk_36, toc->unknown_36, toc->unknown_36_size);
	u8* unk_c9 = RA_dat_writer_lump(writer, LUMP_TOC_UNKNOWN_C9, toc->unknown_c9_size);
	memcpy(unk_c9, toc->unknown_c9, toc->unknown_c9_size);
	u8* unk_62 = RA_dat_writer_lump(writer, LUMP_TOC_UNKNOWN_62, toc->unknown_62_size);
	memcpy(unk_62, toc->unknown_62, toc->unknown_62_size);
	
	u32 header_count = 0;
	for(u32 i = 0; i < toc->asset_count; i++) {
		if(toc->assets[i].has_header) {
			header_count++;
		}
	}
	
	RA_TocAssetHeader* asset_headers = RA_dat_writer_lump(writer, LUMP_ASSET_HEADERS, header_count * sizeof(RA_TocAssetHeader));
	for(u32 i = 0; i < toc->asset_count; i++) {
		if(toc->assets[i].has_header) {
			*asset_headers++ = toc->assets[i].header;
		}
	}
	
	RA_dat_writer_string(writer, "ArchiveTOC");
	
	RA_dat_writer_finish(writer, data_dest, size_dest);
	memcpy(*data_dest, &toc->file_header, sizeof(RA_TocFileHeader));
	return RA_SUCCESS;
}

void RA_toc_free(RA_TableOfContents* toc, ShouldFreeFileData free_file_data) {
	RA_arena_destroy(&toc->arena);
	if(free_file_data == FREE_FILE_DATA && toc->file_data) {
		free(toc->file_data);
	}
}

RA_TocAsset* RA_toc_lookup_asset(RA_TocAsset* assets, u32 asset_count, u64 path_hash, u32 group) {
	s64 first;
	s64 last;
	s64 mid;
	s64 range_begin = -1;
	s64 range_end = -1;
	RA_TocAsset* asset;
	
	// Find the first asset of the specified group.
	first = 0;
	last = (s64) asset_count - 1;
	while(first <= last) {
		mid = (first + last) / 2;
		asset = &assets[mid];
		if(asset->group < group) {
			first = mid + 1;
		} else if(asset->group > group) {
			last = mid - 1;
		} else if(mid == 0 || assets[mid - 1].group < group) {
			range_begin = mid;
			break;
		} else {
			last = mid - 1;
		}
	}
	
	if(range_begin == -1) {
		return NULL;
	}
	
	// Find the last asset of the specified group.
	first = 0;
	last = (s64) asset_count - 1;
	while(first <= last) {
		mid = (first + last) / 2;
		asset = &assets[mid];
		if(asset->group < group) {
			first = mid + 1;
		} else if(asset->group > group) {
			last = mid - 1;
		} else if(mid >= asset_count - 1 || assets[mid + 1].group > group) {
			range_end = mid;
			break;
		} else {
			first = mid + 1;
		}
	}
	
	if(range_end == -1) {
		return NULL;
	}
	
	// Find the asset from that range.
	first = range_begin;
	last = range_end;
	
	while(first <= last) {
		mid = (first + last) / 2;
		asset = &assets[mid];
		if(asset->path_hash < path_hash) {
			first = mid + 1;
		} else if(asset->path_hash > path_hash) {
			last = mid - 1;
		} else {
			return asset;
		}
	}
	
	return NULL;
}
