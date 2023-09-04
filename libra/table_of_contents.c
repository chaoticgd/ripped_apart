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
	
	RA_DatLump* asset_ids = RA_dat_lookup_lump(&dat, LUMP_ARCHIVE_TOC_ASSET_IDS);
	if(asset_ids == NULL) {
		RA_dat_free(&dat, DONT_FREE_FILE_DATA);
		RA_arena_destroy(&toc->arena);
		return RA_FAILURE("asset hash lump not found");
	}
	
	RA_DatLump* archive_file = RA_dat_lookup_lump(&dat, LUMP_ARCHIVE_TOC_FILE_METADATA);
	if(archive_file == NULL) {
		RA_dat_free(&dat, DONT_FREE_FILE_DATA);
		RA_arena_destroy(&toc->arena);
		return RA_FAILURE("archive file lump not found");
	}
	
	RA_DatLump* asset_metadata = RA_dat_lookup_lump(&dat, LUMP_ARCHIVE_TOC_ASSET_METADATA);
	if(asset_metadata == NULL) {
		RA_dat_free(&dat, DONT_FREE_FILE_DATA);
		RA_arena_destroy(&toc->arena);
		return RA_FAILURE("file locations lump not found");
	}
	
	toc->archive_count = archive_file->size / sizeof(RA_TocArchive);
	toc->archives = (RA_TocArchive*) archive_file->data;
	
	toc->asset_count = asset_metadata->size / sizeof(RA_TocAssetMetadata);
	toc->assets = RA_arena_alloc(&toc->arena, toc->asset_count * sizeof(RA_TocAsset));
	if(toc->assets == NULL) {
		RA_dat_free(&dat, DONT_FREE_FILE_DATA);
		RA_arena_destroy(&toc->arena);
		return RA_FAILURE("arena allocation failed");
	}
	
	for(u32 i = 0; i < toc->asset_count; i++) {
		toc->assets[i].metadata = ((RA_TocAssetMetadata*) asset_metadata->data)[i];
		toc->assets[i].path_hash = ((u64*) asset_ids->data)[i];
	}
	
	RA_DatLump* asset_groups = RA_dat_lookup_lump(&dat, LUMP_ARCHIVE_TOC_HEADER);
	if(asset_metadata == NULL) {
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
	
	RA_DatLump* texture_asset_ids = RA_dat_lookup_lump(&dat, LUMP_ARCHIVE_TOC_TEXTURE_ASSET_IDS);
	if(texture_asset_ids == NULL) {
		RA_dat_free(&dat, DONT_FREE_FILE_DATA);
		RA_arena_destroy(&toc->arena);
		return RA_FAILURE("texture asset ids lump not found");
	}
	
	RA_DatLump* texture_meta = RA_dat_lookup_lump(&dat, LUMP_ARCHIVE_TOC_TEXTURE_META);
	if(texture_meta == NULL) {
		RA_dat_free(&dat, DONT_FREE_FILE_DATA);
		RA_arena_destroy(&toc->arena);
		return RA_FAILURE("texture meta lump not found");
	}
	
	RA_DatLump* texture_header = RA_dat_lookup_lump(&dat, LUMP_ARCHIVE_TOC_TEXTURE_HEADER);
	if(texture_header == NULL) {
		RA_dat_free(&dat, DONT_FREE_FILE_DATA);
		RA_arena_destroy(&toc->arena);
		return RA_FAILURE("texture header lump not found");
	}
	
	u32 texture_count = *(u32*) texture_header->data;
	for(u32 i = 0; i < texture_count; i++) {
		u64 hash = ((u64*) texture_asset_ids->data)[i];
		RA_TocAsset* asset = RA_toc_lookup_asset(toc->assets, toc->asset_count, hash, 0);
		if(asset == NULL) {
			RA_dat_free(&dat, DONT_FREE_FILE_DATA);
			RA_arena_destroy(&toc->arena);
			return RA_FAILURE("failed to lookup texture id");
		}
		asset->has_texture_meta = true;
		memcpy(&asset->texture_meta, &((RA_TocTextureMeta*) texture_meta->data)[i], sizeof(RA_TocTextureMeta));
	}
	
	RA_DatLump* asset_headers = RA_dat_lookup_lump(&dat, LUMP_ARCHIVE_TOC_ASSET_HEADER_DATA);
	if(asset_headers == NULL) {
		RA_dat_free(&dat, DONT_FREE_FILE_DATA);
		RA_arena_destroy(&toc->arena);
		return RA_FAILURE("asset header lump not found");
	}
	for(u32 i = 0; i < toc->asset_count; i++) {
		u32 header_ofs = toc->assets[i].metadata.header_offset;
		if(header_ofs != 0xffffffff) {
			toc->assets[i].has_header = true;
			memcpy(&toc->assets[i].header, asset_headers->data + toc->assets[i].metadata.header_offset, sizeof(RA_TocAssetHeader));
		}
	}
	
	RA_dat_free(&dat, DONT_FREE_FILE_DATA);
	return RA_SUCCESS;
}

static int compare_toc_assets(const void* lhs, const void* rhs) {
	if(((RA_TocAsset*) lhs)->group != ((RA_TocAsset*) rhs)->group) {
		u32 lhs_group = ((RA_TocAsset*) lhs)->group;
		u32 rhs_group = ((RA_TocAsset*) rhs)->group;
		if(lhs_group < rhs_group) {
			return -1;
		} else if(lhs_group > rhs_group) {
			return 1;
		} else {
			return 0;
		}
	} else {
		u64 lhs_path_hash = ((RA_TocAsset*) lhs)->path_hash;
		u64 rhs_path_hash = ((RA_TocAsset*) rhs)->path_hash;
		if(lhs_path_hash < rhs_path_hash) {
			return -1;
		} else if(lhs_path_hash > rhs_path_hash) {
			return 1;
		} else {
			return 0;
		}
	}
}

RA_Result RA_toc_build(RA_TableOfContents* toc, u8** data_dest, s64* size_dest) {
	RA_Result result;
	
	qsort(toc->assets, toc->asset_count, sizeof(RA_TocAsset), compare_toc_assets);
	
	u32 group_count = 256;
	for(u32 i = 0; i < toc->asset_count; i++) {
		group_count = MAX(toc->assets[i].group + 1, group_count);
	}
	
	if(group_count > 65535) {
		return RA_FAILURE("asset has bad group/span");
	}
	
	u32 header_count = 0;
	for(u32 i = 0; i < toc->asset_count; i++) {
		if(toc->assets[i].has_header) {
			header_count++;
		}
	}
	
	u32 texture_count = 0;
	for(u32 i = 0; i < toc->asset_count; i++) {
		if(toc->assets[i].has_texture_meta) {
			texture_count++;
		}
	}
	
	RA_DatWriter* writer = RA_dat_writer_begin(RA_ASSET_TYPE_TOC, 0x8);
	if(writer == NULL) {
		return RA_FAILURE("cannot allocate dat writer");
	}
	
	RA_TocAssetGroup* asset_groups = RA_dat_writer_lump(writer, LUMP_ARCHIVE_TOC_HEADER, group_count * sizeof(RA_TocAssetGroup));
	u64* asset_ids = RA_dat_writer_lump(writer, LUMP_ARCHIVE_TOC_ASSET_IDS, toc->asset_count * sizeof(u64));
	RA_TocAssetMetadata* asset_metadata = RA_dat_writer_lump(writer, LUMP_ARCHIVE_TOC_ASSET_METADATA, toc->asset_count * sizeof(RA_TocAssetMetadata));
	RA_TocArchive* archives = RA_dat_writer_lump(writer, LUMP_ARCHIVE_TOC_FILE_METADATA, toc->archive_count * sizeof(RA_TocArchive));
	u64* texture_asset_ids = RA_dat_writer_lump(writer, LUMP_ARCHIVE_TOC_TEXTURE_ASSET_IDS, texture_count * sizeof(u64));
	RA_TocTextureMeta* texture_meta = RA_dat_writer_lump(writer, LUMP_ARCHIVE_TOC_TEXTURE_META, texture_count * sizeof(RA_TocTextureMeta));
	u32* texture_header = RA_dat_writer_lump(writer, LUMP_ARCHIVE_TOC_TEXTURE_HEADER, 4);
	RA_TocAssetHeader* asset_headers = RA_dat_writer_lump(writer, LUMP_ARCHIVE_TOC_ASSET_HEADER_DATA, header_count * sizeof(RA_TocAssetHeader));
	u32 archive_toc_string_offset = RA_dat_writer_string(writer, "ArchiveTOC");
	
	b8 allocation_failed =
		asset_groups == NULL ||
		asset_ids == NULL ||
		asset_metadata == NULL ||
		archives == NULL ||
		texture_asset_ids == NULL ||
		texture_meta == NULL ||
		texture_header == NULL ||
		asset_headers == NULL ||
		archive_toc_string_offset == 0;
	if(allocation_failed) {
		RA_dat_writer_abort(writer);
		return RA_FAILURE("cannot allocate lumps");
	}
	
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
	
	for(u32 i = 0; i < toc->asset_count; i++) {
		asset_ids[i] = toc->assets[i].path_hash;
	}
	
	u32 next_header_offset = 0;
	for(u32 i = 0; i < toc->asset_count; i++) {
		asset_metadata[i] = toc->assets[i].metadata;
		if(toc->assets[i].has_header) {
			asset_metadata[i].header_offset = next_header_offset;
			next_header_offset += sizeof(RA_TocAssetHeader);
		}
	}
	
	memcpy(archives, toc->archives, toc->archive_count * sizeof(RA_TocArchive));
	
	u32 texture_index = 0;
	for(u32 i = 0; i < toc->asset_count; i++) {
		if(toc->assets[i].has_texture_meta) {
			texture_asset_ids[texture_index] = toc->assets[i].path_hash;
			memcpy(&texture_meta[texture_index], &toc->assets[i].texture_meta, sizeof(RA_TocTextureMeta));
			texture_index++;
		}
	}
	*texture_header = texture_count;
	
	for(u32 i = 0; i < toc->asset_count; i++) {
		if(toc->assets[i].has_header) {
			*asset_headers++ = toc->assets[i].header;
		}
	}
	
	if((result = RA_dat_writer_finish(writer, data_dest, size_dest)) != RA_SUCCESS) {
		RA_dat_writer_abort(writer);
		return RA_FAILURE(result->message);
	}
	
	
	memcpy(*data_dest, &toc->file_header, sizeof(RA_TocFileHeader));
	return RA_SUCCESS;
}

void RA_toc_free(RA_TableOfContents* toc, ShouldFreeFileData free_file_data) {
	RA_arena_destroy(&toc->arena);
	if(free_file_data == FREE_FILE_DATA && toc->file_data) {
		RA_free(toc->file_data);
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
