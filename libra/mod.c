#include "mod.h"

#include <zip.h>
#include <json_object.h>
#include <json_tokener.h>

#include "platform.h"

typedef struct {
	RA_TocAsset toc;
} RA_LoadedModAsset;

typedef struct {
	char archive_path[0x42];
	RA_LoadedModAsset* assets;
	u32 asset_count;
} RA_LoadedMod;

static RA_Result parse_mod(RA_Mod* mod, const char* game_dir, const char* mod_file_name);
static void free_mod(RA_Mod* mod);
static RA_Result delete_old_cache_files(const char* cache_dir);
static RA_Result load_mod(RA_LoadedMod* dest, RA_Mod* src, const char* mod_path, const char* cache_path);
static RA_Result update_table_of_contents(RA_LoadedMod* mods, u32 mod_count, RA_TableOfContents* toc);
static RA_Result parse_stage(RA_Mod* mod, const char* game_dir, const char* mod_file_name);
static RA_Result load_stage(RA_LoadedMod* dest, RA_Mod* src, const char* mod_path, const char* cache_path);
static RA_Result parse_stage_info(RA_Mod* mod, RA_StringList* headerless, zip_t* in_archive);
static RA_Result parse_stage_entry(RA_LoadedMod* mod, zip_t* in_archive, s64 index, FILE* out_file, RA_StringList* headerless);
static RA_Result parse_rcmod(RA_Mod* mod, const char* game_dir, const char* mod_file_name);
static RA_Result load_rcmod(RA_LoadedMod* dest, RA_Mod* src, const char* mod_path, const char* cache_path);
static char* read_rcmod_string(FILE* file);
static b8 skip_rcmod_string(FILE* file);

static int compare_mods(const void* lhs, const void* rhs) {
	return strcmp(((RA_Mod*) lhs)->file_name, ((RA_Mod*) rhs)->file_name);
}

RA_Result RA_mod_list_load(RA_Mod** mods_dest, u32* mod_count_dest, const char* game_dir, ModLoadErrorFunc* error_func) {
	RA_Result result;
	
	char mods_dir[RA_MAX_PATH];
	if(snprintf(mods_dir, RA_MAX_PATH, "%s/mods", game_dir) < 0) {
		return RA_FAILURE("path too long");
	}
	
	RA_StringList file_names;
	if((result = RA_enumerate_directory(&file_names, mods_dir)) != RA_SUCCESS) {
		return result;
	}
	
	RA_Mod* mods = calloc(file_names.count, sizeof(RA_Mod));
	if(mods == NULL) {
		RA_string_list_destroy(&file_names);
		return RA_FAILURE("failed to allocate mod list");
	}
	
	u32 mod_count = 0;
	for(u32 i = 0; i < file_names.count; i++) {
		if((result = parse_mod(&mods[mod_count], game_dir, file_names.strings[i])) == RA_SUCCESS) {
			mod_count++;
		} else {
			if(strcmp(result->message, "unsupported format") != 0) {
				error_func(file_names.strings[i], result);
			}
		}
	}
	
	qsort(mods, mod_count, sizeof(RA_Mod), compare_mods);
	
	*mods_dest = mods;
	*mod_count_dest = mod_count;
	
	RA_string_list_destroy(&file_names);
	return RA_SUCCESS;
}

void RA_mod_list_free(RA_Mod* mods, u32 mod_count) {
	for(u32 i = 0; i < mod_count; i++) {
		free_mod(&mods[i]);
	}
	free(mods);
}

RA_Result parse_mod(RA_Mod* mod, const char* game_dir, const char* mod_file_name) {
	const char* extension = strrchr(mod_file_name, '.');
	if(extension != NULL) {
		if(strcmp(extension, ".stage") == 0) {
			return parse_stage(mod, game_dir, mod_file_name);
		}
		
		if(strcmp(extension, ".rcmod") == 0) {
			return parse_rcmod(mod, game_dir, mod_file_name);
		}
	}
	
	return RA_FAILURE("unsupported format");
}

static void free_mod(RA_Mod* mod) {
	if(mod->name != NULL) free(mod->name);
	if(mod->version != NULL) free(mod->version);
	if(mod->description != NULL) free(mod->description);
	if(mod->author != NULL) free(mod->author);
}

// *****************************************************************************

RA_Result RA_install_mods(RA_Mod* mods, u32 mod_count, RA_TableOfContents* toc, u32* success_count_dest, u32* fail_count_dest, const char* game_dir, ModLoadErrorFunc* error_func) {
	RA_Result result;
	
	// Fail safe to make sure we don't delete any .cache files from the wrong
	// directory, probably overkill but better to be careful.
	char exe_path[RA_MAX_PATH];
	if(snprintf(exe_path, sizeof(exe_path), "%s/RiftApart.exe", game_dir) < 0) {
		return RA_FAILURE("game folder path too long");
	}
	if(!RA_file_exists(exe_path)) {
		return RA_FAILURE("RiftApart.exe not found");
	}
	
	char cache_dir[RA_MAX_PATH];
	if(snprintf(cache_dir, sizeof(cache_dir), "%s/modcache", game_dir) < 0) {
		return RA_FAILURE("path too long");
	}
	if((result = delete_old_cache_files(cache_dir)) != RA_SUCCESS) {
		return RA_FAILURE(result->message);
	}
	
	RA_LoadedMod* loaded_mods = malloc(mod_count * sizeof(RA_LoadedMod));
	u32 loaded_mod_count = 0;
	u32 success_count = 0;
	u32 fail_count = 0;
	if(loaded_mods == NULL) {
		return RA_FAILURE("cannot allocate mod list");
	}
	for(u32 i = 0; i < mod_count; i++) {
		if(mods[i].enabled) {
			char mod_path[RA_MAX_PATH];
			if(snprintf(mod_path, sizeof(mod_path), "%s/mods/%s", game_dir, mods[i].file_name) < 0) {
				return RA_FAILURE("path too long");
			}
			
			char cache_path[RA_MAX_PATH];
			if(snprintf(cache_path, sizeof(cache_path), "%s/%s.cache", cache_dir, mods[i].file_name) < 0) {
				return RA_FAILURE("path too long");
			}
			
			if((result = load_mod(&loaded_mods[loaded_mod_count], &mods[i], mod_path, cache_path)) == RA_SUCCESS) {
				loaded_mod_count++;
				success_count++;
			} else {
				error_func(mods[i].file_name, result);
				fail_count++;
			}
		}
	}
	
	if((result = update_table_of_contents(loaded_mods, loaded_mod_count, toc)) != RA_SUCCESS) {
		free(loaded_mods);
		return result;
	}
	
	*success_count_dest = success_count;
	*fail_count_dest = fail_count;
	
	return RA_SUCCESS;
}

static RA_Result delete_old_cache_files(const char* cache_dir) {
	RA_Result result;
	
	RA_StringList file_names;
	if((result = RA_enumerate_directory(&file_names, cache_dir)) != RA_SUCCESS) {
		return result;
	}
	
	for(u32 i = 0; i < file_names.count; i++) {
		const char* file_name = file_names.strings[i];
		const char* extension = strrchr(file_name, '.');
		if(strcmp(extension, ".cache") == 0) {
			char file_path[RA_MAX_PATH];
			if(snprintf(file_path, sizeof(file_path), "%s/%s", cache_dir, file_name) < 0) {
				RA_string_list_destroy(&file_names);
				return RA_FAILURE("mod cache path too long");
			}
			remove(file_path);
		}
	}
	
	RA_string_list_destroy(&file_names);
	
	return RA_SUCCESS;
}

static RA_Result load_mod(RA_LoadedMod* dest, RA_Mod* src, const char* mod_path, const char* cache_path) {
	switch(src->format) {
		case RA_MOD_FORMAT_STAGE: return load_stage(dest, src, mod_path, cache_path);
		case RA_MOD_FORMAT_RCMOD: return load_rcmod(dest, src, mod_path, cache_path);
		default: return RA_FAILURE("invalid format enum");
	}
}

static RA_Result update_table_of_contents(RA_LoadedMod* mods, u32 mod_count, RA_TableOfContents* toc) {
	// Add archives.
	RA_TocArchive* new_archives = RA_arena_alloc(&toc->arena, (toc->archive_count + mod_count) * sizeof(RA_TocArchive));
	if(new_archives == NULL) {
		return RA_FAILURE("allocation failed");
	}
	memcpy(new_archives, toc->archives, toc->archive_count * sizeof(RA_TocArchive));
	memset(new_archives + toc->archive_count, 0, mod_count * sizeof(RA_TocArchive));
	
	u32 old_archive_count = toc->archive_count;
	toc->archives = new_archives;
	
	for(u32 i = 0; i < mod_count; i++) {
		RA_TocArchive* archive = &toc->archives[toc->archive_count];
		RA_string_copy(archive->data, mods[i].archive_path, sizeof(archive->data));
		toc->archive_count++;
	}
	
	// Add assets.
	u32 max_asset_count = toc->asset_count;
	for(u32 i = 0; i < mod_count; i++) {
		max_asset_count += mods[i].asset_count;
	}
	
	RA_TocAsset* new_assets = RA_arena_alloc(&toc->arena, max_asset_count * sizeof(RA_TocAsset));
	if(new_assets == NULL) {
		return RA_FAILURE("allocation failed");
	}
	memcpy(new_assets, toc->assets, toc->asset_count * sizeof(RA_TocAsset));
	memset(new_assets + toc->asset_count, 0, (max_asset_count - toc->asset_count) * sizeof(RA_TocAsset));
	
	u32 old_asset_count = toc->asset_count;
	toc->assets = new_assets;
	
	u32 archive_index = old_archive_count;
	for(u32 i = 0; i < mod_count; i++) {
		for(u32 j = 0; j < mods[i].asset_count; j++) {
			RA_LoadedModAsset* mod_asset = &mods[i].assets[j];
			RA_TocAsset* toc_asset = RA_toc_lookup_asset(toc->assets, old_asset_count, mod_asset->toc.path_hash, mod_asset->toc.group);
			if(toc_asset) {
				*toc_asset = mod_asset->toc;
			} else {
				toc_asset = &toc->assets[toc->asset_count];
				toc->asset_count++;
			}
			*toc_asset = mod_asset->toc;
			toc_asset->location.archive_index = archive_index;
		}
		archive_index++;
	}
	
	return RA_SUCCESS;
}

// *****************************************************************************

static RA_Result parse_stage(RA_Mod* mod, const char* game_dir, const char* mod_file_name) {
	RA_Result result;
	
	memset(mod, 0, sizeof(RA_Mod));
	RA_string_copy(mod->file_name, mod_file_name, sizeof(mod->file_name));
	
	// Build the file path.
	char absolute_mod_path[RA_MAX_PATH];
	if(snprintf(absolute_mod_path, RA_MAX_PATH, "%s/mods/%s", game_dir, mod_file_name) < 0) {
		return RA_FAILURE("path too long");
	}
	
	// Open the .stage zip archive.
	int err;
	struct zip* in_archive = zip_open(absolute_mod_path, 0, &err);
	if(in_archive == NULL) {
		zip_error_t error;
		zip_error_init_with_code(&error, err);
		RA_Result result_val = RA_FAILURE("cannot open zip archive: %s", zip_error_strerror(&error));
		zip_error_fini(&error);
		return result_val;
	}
	
	// Read metadata.
	if((result = parse_stage_info(mod, NULL, in_archive)) != RA_SUCCESS) {
		zip_close(in_archive);
		return RA_FAILURE("cannot read info.json: %s", result->message);
	}
	
	zip_close(in_archive);
	
	mod->format = RA_MOD_FORMAT_STAGE;
	return RA_SUCCESS;
}

static RA_Result load_stage(RA_LoadedMod* dest, RA_Mod* src, const char* mod_path, const char* cache_path) {
	RA_Result result;
	
	memset(dest, 0, sizeof(RA_LoadedMod));
	
	// Open the .stage zip archive.
	int err;
	struct zip* in_archive = zip_open(mod_path, 0, &err);
	if(in_archive == NULL) {
		zip_error_t error;
		zip_error_init_with_code(&error, err);
		RA_Result result_val = RA_FAILURE("cannot open zip archive: %s", zip_error_strerror(&error));
		zip_error_fini(&error);
		return result_val;
	}
	
	// Open the cache file, which we'll use to store the decompressed assets
	// that the game will actually use.
	RA_make_dirs(cache_path);
	FILE* out_file = fopen(cache_path, "wb");
	if(out_file == NULL) {
		zip_close(in_archive);
		return RA_FAILURE("cannot open cache file");
	}
	
	// Determine which files lack the "header" that would be stored in the toc.
	RA_StringList headerless;
	if((result = parse_stage_info(NULL, &headerless, in_archive)) != RA_SUCCESS) {
		fclose(out_file);
		zip_close(in_archive);
		return RA_FAILURE("cannot read info.json: %s", result->message);
	}
	
	s64 entry_count = zip_get_num_entries(in_archive, 0);
	if(entry_count == -1) {
		RA_string_list_destroy(&headerless);
		fclose(out_file);
		zip_close(in_archive);
		return RA_FAILURE("zip_get_num_entries returned -1");
	}
	dest->assets = calloc(entry_count, sizeof(RA_LoadedModAsset));
	if(dest->assets == NULL) {
		RA_string_list_destroy(&headerless);
		fclose(out_file);
		zip_close(in_archive);
		return RA_FAILURE("cannot allocate asset list");
	}
	
	for(s64 i = 0; i < entry_count; i++) {
		if((result = parse_stage_entry(dest, in_archive, i, out_file, &headerless)) != RA_SUCCESS) {
			free(dest->assets);
			RA_string_list_destroy(&headerless);
			fclose(out_file);
			zip_close(in_archive);
			return result;
		}
	}
	
	// Cleanup.
	RA_string_list_destroy(&headerless);
	fclose(out_file);
	zip_close(in_archive);
	
	if(snprintf(dest->archive_path, sizeof(dest->archive_path), "modcache\\%s.cache", src->file_name) < 0) {
		free(dest->assets);
		return RA_FAILURE("path too long");
	}
	
	return RA_SUCCESS;
}

static RA_Result parse_stage_info(RA_Mod* mod, RA_StringList* headerless, zip_t* in_archive) {
	RA_Result result;
	
	zip_stat_t stat;
	if(zip_stat(in_archive, "info.json", 0, &stat) != 0 || !(stat.valid & ZIP_STAT_NAME) || !(stat.valid & ZIP_STAT_SIZE)) {
		return RA_FAILURE("cannot stat file");
	}
	
	zip_file_t* info_file = zip_fopen_index(in_archive, stat.index, 0);
	if(info_file == NULL) {
		return RA_FAILURE("file missing");
	}
	
	u8* json_data = malloc(stat.size + 1);
	if(json_data == NULL) {
		zip_fclose(info_file);
		return RA_FAILURE("cannot allocate space");
	}
	if(zip_fread(info_file, json_data, stat.size) != stat.size) {
		free(json_data);
		zip_fclose(info_file);
		return RA_FAILURE("cannot read");
	}
	json_data[stat.size] = '\0';
	
	json_object* root = json_tokener_parse((char*) json_data);
	if(root == NULL) {
		free(json_data);
		zip_fclose(info_file);
		return RA_FAILURE("parsing failed");
	}
	free(json_data);
	
	if(mod != NULL) {
		json_object* name_json = json_object_object_get(root, "name");
		if(name_json != NULL) {
			const char* name_str = json_object_get_string(name_json);
			if(name_str != NULL) {
				mod->name = malloc(strlen(name_str) + 1);
				if(mod->name != NULL) {
					RA_string_copy(mod->name, name_str, strlen(name_str) + 1);
				}
			}
		}
		
		json_object* author_json = json_object_object_get(root, "author");
		if(author_json != NULL) {
			const char* author_str = json_object_get_string(author_json);
			if(author_str != NULL) {
				mod->author = malloc(strlen(author_str) + 1);
				if(mod->author != NULL) {
					RA_string_copy(mod->author, author_str, strlen(author_str) + 1);
				}
			}
		}
	}
	
	if(headerless != NULL) {
		RA_string_list_create(headerless);
		
		json_object* headerless_json = json_object_object_get(root, "headerless");
		if(headerless_json != NULL && json_object_is_type(headerless_json, json_type_array)) {
			size_t count = json_object_array_length(headerless_json);
			for(size_t i = 0; i < count; i++) {
				json_object* element = json_object_array_get_idx(headerless_json, i);
				const char* headerless_path = json_object_get_string(element);
				if(headerless_path == NULL) {
					json_object_put(root);
					zip_fclose(info_file);
					return RA_FAILURE("bad headerless property");
				}
				if((result = RA_string_list_add(headerless, headerless_path)) != RA_SUCCESS) {
					json_object_put(root);
					zip_fclose(info_file);
					return result;
				}
			}
		}
		
		if((result = RA_string_list_finish(headerless)) != RA_SUCCESS) {
			RA_string_list_destroy(headerless);
			json_object_put(root);
			zip_fclose(info_file);
			return result;
		}
	}
	
	json_object_put(root);
	zip_fclose(info_file);
	
	return RA_SUCCESS;
}

static RA_Result parse_stage_entry(RA_LoadedMod* mod, zip_t* in_archive, s64 index, FILE* out_file, RA_StringList* headerless) {
	zip_stat_t stat;
	if(zip_stat_index(in_archive, index, 0, &stat) != 0 || !(stat.valid & ZIP_STAT_NAME) || !(stat.valid & ZIP_STAT_SIZE)) {
		return RA_FAILURE("cannot stat entry %d", index);
	}
	
	const char* name = stat.name;
	if(name[strlen(name) - 1] == '/') {
		// Not an asset.
		return RA_SUCCESS;
	}
	
	char* relative_path;
	u32 group = (u32) strtoll(name, &relative_path, 10);
	if(relative_path == name || relative_path == NULL || *relative_path == '\0') {
		// Not an asset.
		return RA_SUCCESS;
	}
	relative_path++; // Skip past '/'.
	
	b8 is_hash_path = true;
	if(strlen(relative_path) == 16) {
		for(u32 i = 0; i < 16; i++) {
			b8 is_digit = relative_path[i] >= '0' && relative_path[i] <= '9';
			b8 is_upper_hex = relative_path[i] >= 'A' && relative_path[i] <= 'F';
			b8 is_lower_hex = relative_path[i] >= 'a' && relative_path[i] <= 'f';
			if(!is_digit && !is_upper_hex && !is_lower_hex) {
				is_hash_path = false;
			}
		}
	} else {
		is_hash_path = false;
	}
	
	RA_LoadedModAsset* asset = &mod->assets[mod->asset_count++];
	asset->toc.path_hash = is_hash_path ? strtoull(relative_path, NULL, 16) : RA_crc64_path(relative_path);
	asset->toc.group = group;
	
	b8 has_header = true;
	for(u32 i = 0; i < headerless->count; i++) {
		if(strcmp(headerless->strings[i], name) == 0) {
			has_header = false;
			break;
		}
	}
	
	zip_file_t* file = zip_fopen_index(in_archive, index, 0);
	
	u32 header_size = 0;
	if(has_header) {
		asset->toc.has_header = true;
		if(zip_fread(file, &asset->toc.header, sizeof(RA_TocAssetHeader)) != sizeof(RA_TocAssetHeader)) {
			zip_fclose(file);
			return RA_FAILURE("cannot read asset header for asset %s", name);
		}
		header_size = sizeof(RA_TocAssetHeader);
	}
	
	u32 file_size = stat.size - header_size;
	u32 allocation_size = ALIGN(file_size, 0x40);
	u8* file_data = calloc(1, allocation_size);
	if(file_data == NULL) {
		zip_fclose(file);
		return RA_FAILURE("cannot allocate file data for asset %s", name);
	}
	if(zip_fread(file, file_data, file_size) != file_size) {
		free(file_data);
		zip_fclose(file);
		return RA_FAILURE("cannot read data for asset %s", name);
	}
	
	s64 begin_offset = ftell(out_file);
	if(fwrite(file_data, allocation_size, 1, out_file) != 1) {
		free(file_data);
		zip_fclose(file);
		return RA_FAILURE("cannot write data to cache file for asset %s", name);
	}
	s64 end_offset = begin_offset + file_size;
	
	asset->toc.location.offset = (u32) begin_offset;
	asset->toc.location.size = (u32) (end_offset - begin_offset);
	
	free(file_data);
	zip_fclose(file);
	
	return RA_SUCCESS;
}

typedef struct {
	u32 offset;
	u32 size;
	u64 hash;
	u16 group;
} RCMOD_DirEntry;
#define RCMOD_DIRENTRY_SIZE_ON_DISK 0x12

static RA_Result parse_rcmod(RA_Mod* mod, const char* game_dir, const char* mod_file_name) {
	memset(mod, 0, sizeof(RA_Mod));
	RA_string_copy(mod->file_name, mod_file_name, sizeof(mod->file_name));
	
	char absolute_mod_path[RA_MAX_PATH];
	snprintf(absolute_mod_path, RA_MAX_PATH, "%s/mods/%s", game_dir, mod_file_name);
	
	FILE* file = fopen(absolute_mod_path, "rb");
	if(file == NULL) {
		return RA_FAILURE("fopen");
	}
	
	s64 file_size = RA_file_size(file);
	if(file_size == -1) {
		fclose(file);
		free_mod(mod);
		return RA_FAILURE("cannot determine file size");
	}
	
	mod->name = read_rcmod_string(file);
	mod->version = read_rcmod_string(file);
	mod->description = read_rcmod_string(file);
	mod->author = read_rcmod_string(file);
	if(mod->name == NULL || mod->version == NULL || mod->description == NULL || mod->author == NULL) {
		fclose(file);
		free_mod(mod);
		return RA_FAILURE("cannot read header");
	}
	
	fclose(file);
	
	mod->format = RA_MOD_FORMAT_RCMOD;
	return RA_SUCCESS;
}

static RA_Result load_rcmod(RA_LoadedMod* dest, RA_Mod* src, const char* mod_path, const char* cache_path) {
	memset(dest, 0, sizeof(RA_LoadedMod));
	
	FILE* file = fopen(mod_path, "rb");
	if(file == NULL) {
		return RA_FAILURE("fopen");
	}
	
	s64 file_size = RA_file_size(file);
	if(file_size == -1) {
		fclose(file);
		return RA_FAILURE("cannot determine file size");
	}
	
	b8 name = skip_rcmod_string(file);
	b8 version = skip_rcmod_string(file);
	b8 description = skip_rcmod_string(file);
	b8 author = skip_rcmod_string(file);
	if(!name || !version || ! description || !author) {
		fclose(file);
		return RA_FAILURE("cannot read header");
	}
	
	u32 directory_offset = 0;
	if(fread(&directory_offset, 4, 1, file) != 1) {
		return RA_FAILURE("cannot read header");
	}
	
	dest->asset_count = (file_size - directory_offset) / RCMOD_DIRENTRY_SIZE_ON_DISK;
	dest->assets = calloc(dest->asset_count, sizeof(RA_LoadedModAsset));
	if(dest->assets == NULL) {
		fclose(file);
		return RA_FAILURE("cannot allocate assets list");
	}
	
	if(fseek(file, directory_offset, SEEK_SET) != 0) {
		free(dest->assets);
		fclose(file);
		return RA_FAILURE("cannot seek to directory entries");
	}
	RCMOD_DirEntry* entries = malloc(dest->asset_count * sizeof(RCMOD_DirEntry));
	if(entries == NULL) {
		free(dest->assets);
		fclose(file);
		return RA_FAILURE("cannot allocate directory entries");
	}
	for(u32 i = 0; i < dest->asset_count; i++) {
		if(fread(&entries[i], RCMOD_DIRENTRY_SIZE_ON_DISK, 1, file) != 1) {
			free(entries);
			free(dest->assets);
			fclose(file);
			return RA_FAILURE("cannot read directory entries");
		}
	}
	
	for(u32 i = 0; i < dest->asset_count; i++) {
		dest->assets[i].toc.has_header = true;
		if(fseek(file, entries[i].offset, SEEK_SET) != 0) {
			free(entries);
			free(dest->assets);
			fclose(file);
			return RA_FAILURE("can't seek to asset header");
		}
		if(fread(&dest->assets[i].toc.header, sizeof(RA_TocAssetHeader), 1, file) != 1) {
			free(entries);
			free(dest->assets);
			fclose(file);
			return RA_FAILURE("can't read asset header");
		}
		dest->assets[i].toc.location.offset = entries[i].offset + sizeof(RA_TocAssetHeader);
		dest->assets[i].toc.location.size = entries[i].size - sizeof(RA_TocAssetHeader);
		dest->assets[i].toc.path_hash = entries[i].hash;
		dest->assets[i].toc.group = entries[i].group;
	}
	
	free(entries);
	fclose(file);
	
	if(snprintf(dest->archive_path, sizeof(dest->archive_path), "mods\\%s", src->file_name) < 0) {
		free(dest->assets);
		return RA_FAILURE("path too long");
	}
	
	return RA_SUCCESS;
}

static char* read_rcmod_string(FILE* file) {
	static char* scratchpad = NULL;
	if(scratchpad == NULL) {
		scratchpad = malloc(1024 * 1024);
		if(scratchpad == NULL) {
			return NULL;
		}
	}
	for(s32 i = 0; i < 1024 * 1024; i++) {
		if(fread(&scratchpad[i], 1, 1, file) != 1) {
			return NULL;
		}
		if(scratchpad[i] == '\0') {
			char* string = malloc(i + 1);
			if(string == NULL) {
				return NULL;
			}
			memcpy(string, scratchpad, i + 1);
			return string;
		}
	}
	return NULL;
}

static b8 skip_rcmod_string(FILE* file) {
	for(s32 i = 0; i < 1024 * 1024; i++) {
		char c;
		if(fread(&c, 1, 1, file) != 1) {
			return false;
		}
		if(c == '\0') {
			return true;
		}
	}
	return false;
}
