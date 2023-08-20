#include "mod.h"

#include "platform.h"

RA_Result RA_mod_list_load(RA_Mod** mods, u32* mod_count, const char* game_dir) {
	RA_Result result;
	
	char mods_dir[RA_MAX_PATH];
	snprintf(mods_dir, RA_MAX_PATH, "%s/mods", game_dir);
	
	RA_StringList file_names;
	if((result = RA_enumerate_directory(&file_names, mods_dir)) != RA_SUCCESS) {
		return result;
	}
	
	*mods = calloc(file_names.count, sizeof(RA_Mod));
	*mod_count = file_names.count;
	
	for(u32 i = 0; i < file_names.count; i++) {
		if((result = RA_mod_read(&(*mods)[i], game_dir, file_names.strings[i])) != RA_SUCCESS) {
			return result;
		}
	}
	
	return RA_SUCCESS;
}

RA_Result RA_mod_list_rebuild_toc(RA_Mod* mods, u32 mod_count, RA_TableOfContents* toc) {
	// Add archives.
	RA_TocArchive* new_archives = RA_arena_alloc(&toc->arena, (toc->archive_count + mod_count) * sizeof(RA_TocArchive));
	if(new_archives == NULL) {
		return RA_FAILURE("allocation failed");
	}
	memcpy(new_archives, toc->archives, toc->archive_count * sizeof(RA_TocArchive));
	memset(new_archives + toc->archive_count, 0, mod_count * sizeof(RA_TocArchive));
	
	u32 old_archive_count = toc->archive_count;
	toc->archives = new_archives;
	toc->archive_count += mod_count;
	
	for(u32 i = 0; i < mod_count; i++) {
		RA_TocArchive* archive = &toc->archives[old_archive_count + i];
		RA_string_copy(archive->data, mods[i].archive_path, sizeof(archive->data));
	}
	
	// Add assets.
	u32 max_asset_count = toc->asset_count;
	for(u32 i = 0; i < mod_count; i++) {
		if(mods[i].initialised) {
			max_asset_count += mods[i].asset_count;
		}
	}
	
	RA_TocAsset* new_assets = RA_arena_alloc(&toc->arena, max_asset_count * sizeof(RA_TocAsset));
	if(new_assets == NULL) {
		return RA_FAILURE("allocation failed");
	}
	memcpy(new_assets, toc->assets, toc->asset_count * sizeof(RA_TocAsset));
	memset(new_assets + toc->asset_count, 0, (max_asset_count - toc->asset_count) * sizeof(RA_TocAsset));
	
	u32 old_asset_count = toc->asset_count;
	toc->assets = new_assets;
	
	for(u32 i = 0; i < mod_count; i++) {
		if(mods[i].initialised) {
			for(u32 j = 0; j < mods[i].asset_count; j++) {
				RA_ModAsset* mod_asset = &mods[i].assets[j];
				RA_TocAsset* toc_asset = RA_toc_lookup_asset(toc->assets, old_asset_count, mod_asset->toc.path_hash, mod_asset->toc.group);
				if(toc_asset) {
					*toc_asset = mod_asset->toc;
					toc_asset->location.archive_index = old_archive_count + i;
				} else {
					toc_asset = &toc->assets[toc->asset_count];
					toc->asset_count++;
				}
				*toc_asset = mod_asset->toc;
				toc_asset->location.archive_index = old_archive_count + i;
			}
		}
	}
	
	return RA_SUCCESS;
}

void RA_mod_list_free(RA_Mod* mods, u32 mod_count) {
	for(u32 i = 0; i < mod_count; i++) {
		RA_mod_free(&mods[i]);
	}
	free(mods);
}

static RA_Result read_rcmod(RA_Mod* mod, u8* data, u32 size, const char* archive_path);

RA_Result RA_mod_read(RA_Mod* mod, const char* game_dir, const char* mod_file_name) {
	RA_Result result;
	
	memset(mod, 0, sizeof(RA_Mod));
	
	char absolute_mod_path[RA_MAX_PATH];
	snprintf(absolute_mod_path, RA_MAX_PATH, "%s/mods/%s", game_dir, mod_file_name);
	
	u8* mod_data;
	u32 mod_size;
	if((result = RA_file_read(absolute_mod_path, &mod_data, &mod_size)) != RA_SUCCESS) {
		return result;
	}
	
	char archive_path[RA_MAX_PATH];
	snprintf(archive_path, RA_MAX_PATH, "mods\\%s", mod_file_name);
	
	// The .rcmod files don't have a magic identifier so we check the extension.
	const char* extension = strrchr(absolute_mod_path, '.');
	if(strcmp(extension, ".rcmod") == 0) {
		return read_rcmod(mod, mod_data, mod_size, archive_path);
	}
	
	RA_mod_free(mod);
	return RA_FAILURE("unsupported format");
}

void RA_mod_free(RA_Mod* mod) {
	if(mod->assets != NULL) free(mod->assets);
	if(mod->name != NULL) free(mod->name);
	if(mod->version != NULL) free(mod->version);
	if(mod->description != NULL) free(mod->description);
	if(mod->author != NULL) free(mod->author);
}

typedef struct {
	u32 offset;
	u32 size;
	u64 hash;
	u16 group;
} RCMOD_DirEntry;
#define RCMOD_DIRENTRY_SIZE_ON_DISK 0x12

static s32 eat_rcmod_string(s32 offset, u8* data, u32 size) {
	if(offset == -1) {
		return -1;
	}
	for(s32 i = offset; i < size; i++) {
		if(data[i] == '\0') {
			return i + 1;
		}
	}
	return -1;
}

static RA_Result read_rcmod(RA_Mod* mod, u8* data, u32 size, const char* archive_path) {
	s32 name_offset = 0;
	s32 version_offset = eat_rcmod_string(name_offset, data, size);
	s32 description_offset = eat_rcmod_string(version_offset, data, size);
	s32 author_offset = eat_rcmod_string(description_offset, data, size);
	s32 table_pointer_offset = eat_rcmod_string(author_offset, data, size);
	if(table_pointer_offset == -1) {
		RA_mod_free(mod);
		return RA_FAILURE("rcmod file has invalid header: bad strings");
	}
	
	mod->name = malloc(version_offset - name_offset);
	mod->version = malloc(description_offset - version_offset);
	mod->description = malloc(author_offset - description_offset);
	mod->author = malloc(table_pointer_offset - author_offset);
	memcpy(mod->name, &data[name_offset], version_offset - name_offset);
	memcpy(mod->version, &data[version_offset], description_offset - version_offset);
	memcpy(mod->description, &data[description_offset], author_offset - description_offset);
	memcpy(mod->author, &data[author_offset], table_pointer_offset - author_offset);
	
	if(table_pointer_offset + 4 > size) {
		RA_mod_free(mod);
		return RA_FAILURE("rcmod file has invalid header: end of file before table pointer");
	}
	
	u32 table_offset = 0;
	memcpy(&table_offset, &data[table_pointer_offset], 4);
	
	mod->asset_count = (size - table_offset) / RCMOD_DIRENTRY_SIZE_ON_DISK;
	mod->assets = calloc(mod->asset_count, sizeof(RA_ModAsset));
	
	u32 entry_offset = table_offset;
	for(u32 i = 0; i < mod->asset_count; i++) {
		RCMOD_DirEntry entry;
		memcpy(&entry, &data[entry_offset], RCMOD_DIRENTRY_SIZE_ON_DISK);
		if(entry.offset + sizeof(RA_TocAssetHeader) > size) {
			RA_mod_free(mod);
			return RA_FAILURE("offset beyond end of file");
		}
		mod->assets[i].toc.has_header = true;
		memcpy(&mod->assets[i].toc.header, data + entry.offset, sizeof(RA_TocAssetHeader));
		mod->assets[i].toc.location.offset = entry.offset + sizeof(RA_TocAssetHeader);
		mod->assets[i].toc.location.size = entry.size - sizeof(RA_TocAssetHeader);
		mod->assets[i].toc.path_hash = entry.hash;
		mod->assets[i].toc.group = entry.group;
		entry_offset += RCMOD_DIRENTRY_SIZE_ON_DISK;
	}
	
	RA_string_copy(mod->archive_path, archive_path, sizeof(mod->archive_path));
	mod->initialised = true;
	
	return RA_SUCCESS;
}
