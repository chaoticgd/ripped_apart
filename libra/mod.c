#include "mod.h"

#include <zip.h>
#include <json_object.h>
#include <json_tokener.h>

#include "platform.h"

static int compare_mods(const void* lhs, const void* rhs) {
	return strcmp(((RA_Mod*) lhs)->file_name, ((RA_Mod*) rhs)->file_name);
}

RA_Result RA_mod_list_load(RA_Mod** mods_dest, u32* mod_count_dest, const char* game_dir, ModLoadErrorFunc* error_func) {
	RA_Result result;
	
	char mods_dir[RA_MAX_PATH];
	snprintf(mods_dir, RA_MAX_PATH, "%s/mods", game_dir);
	
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
		if((result = RA_mod_read(&mods[mod_count], game_dir, file_names.strings[i])) == RA_SUCCESS) {
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

RA_Result RA_mod_list_rebuild_toc(RA_Mod* mods, u32 mod_count, RA_TableOfContents* toc, u32* mod_count_dest) {
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
		if(mods[i].enabled) {
			RA_TocArchive* archive = &toc->archives[toc->archive_count];
			RA_string_copy(archive->data, mods[i].archive_path, sizeof(archive->data));
			toc->archive_count++;
		}
	}
	
	// Add assets.
	u32 max_asset_count = toc->asset_count;
	for(u32 i = 0; i < mod_count; i++) {
		if(mods[i].enabled) {
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
	
	if(mod_count_dest) {
		*mod_count_dest = 0;
	}
	
	u32 archive_index = old_archive_count;
	for(u32 i = 0; i < mod_count; i++) {
		if(mods[i].enabled) {
			for(u32 j = 0; j < mods[i].asset_count; j++) {
				RA_ModAsset* mod_asset = &mods[i].assets[j];
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
			
			if(mod_count_dest) {
				(*mod_count_dest)++;
			}
			archive_index++;
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

static RA_Result parse_stage(RA_Mod* mod, const char* game_dir, const char* mod_file_name);
static RA_Result parse_stage_info(RA_Mod* mod, RA_StringList* headerless, zip_t* in_archive);
static RA_Result parse_stage_entry(RA_Mod* mod, zip_t* in_archive, s64 index, FILE* out_file, RA_StringList* headerless);
static RA_Result parse_rcmod(RA_Mod* mod, const char* game_dir, const char* mod_file_name);

RA_Result RA_mod_read(RA_Mod* mod, const char* game_dir, const char* mod_file_name) {
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

void RA_mod_free(RA_Mod* mod) {
	if(mod->assets != NULL) free(mod->assets);
	if(mod->name != NULL) free(mod->name);
	if(mod->version != NULL) free(mod->version);
	if(mod->description != NULL) free(mod->description);
	if(mod->author != NULL) free(mod->author);
}

static RA_Result parse_stage(RA_Mod* mod, const char* game_dir, const char* mod_file_name) {
	RA_Result result;
	
	memset(mod, 0, sizeof(RA_Mod));
	RA_string_copy(mod->file_name, mod_file_name, sizeof(mod->file_name));
	
	// Build the file paths.
	char absolute_mod_path[RA_MAX_PATH];
	if(snprintf(absolute_mod_path, RA_MAX_PATH, "%s/mods/%s", game_dir, mod_file_name) < 0) {
		return RA_FAILURE("path too long");
	}
	
	char absolute_modcache_path[RA_MAX_PATH];
	if(snprintf(absolute_modcache_path, RA_MAX_PATH, "%s/modcache/%s.cache", game_dir, mod_file_name) < 0) {
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
	
	// Open the cache file, which we'll use to store the decompressed assets
	// that the game will actually use.
	RA_make_dirs(absolute_modcache_path);
	FILE* out_file = fopen(absolute_modcache_path, "wb");
	if(out_file == NULL) {
		zip_close(in_archive);
		return RA_FAILURE("cannot open cache file");
	}
	
	// Read metadata and determine which files lack the "header" that would be
	// stored in the toc.
	RA_StringList headerless;
	if((result = parse_stage_info(mod, &headerless, in_archive)) != RA_SUCCESS) {
		RA_mod_free(mod);
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
	mod->assets = calloc(entry_count, sizeof(RA_ModAsset));
	
	for(s64 i = 0; i < entry_count; i++) {
		if((result = parse_stage_entry(mod, in_archive, i, out_file, &headerless)) != RA_SUCCESS) {
			RA_mod_free(mod);
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
	
	if(snprintf(mod->archive_path, sizeof(mod->archive_path), "modcache\\%s.cache", mod_file_name) < 0) {
		RA_mod_free(mod);
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
	
	json_object_put(root);
	zip_fclose(info_file);
	
	if((result = RA_string_list_finish(headerless))) {
		return result;
	}
	
	return RA_SUCCESS;
}

static RA_Result parse_stage_entry(RA_Mod* mod, zip_t* in_archive, s64 index, FILE* out_file, RA_StringList* headerless) {
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
	
	RA_ModAsset* asset = &mod->assets[mod->asset_count++];
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
		RA_mod_free(mod);
		return RA_FAILURE("cannot determine file size");
	}
	
	mod->name = read_rcmod_string(file);
	mod->version = read_rcmod_string(file);
	mod->description = read_rcmod_string(file);
	mod->author = read_rcmod_string(file);
	if(mod->name == NULL || mod->version == NULL || mod->description == NULL || mod->author == NULL) {
		RA_mod_free(mod);
		return RA_FAILURE("cannot read header");
	}
	
	u32 directory_offset = 0;
	if(fread(&directory_offset, 4, 1, file) != 1) {
		RA_mod_free(mod);
		return RA_FAILURE("cannot read header");
	}
	
	mod->asset_count = (file_size - directory_offset) / RCMOD_DIRENTRY_SIZE_ON_DISK;
	mod->assets = calloc(mod->asset_count, sizeof(RA_ModAsset));
	
	if(fseek(file, directory_offset, SEEK_SET) != 0) {
		RA_mod_free(mod);
		return RA_FAILURE("can't seek to directory entries");
	}
	RCMOD_DirEntry* entries = malloc(mod->asset_count * sizeof(RCMOD_DirEntry));
	for(u32 i = 0; i < mod->asset_count; i++) {
		if(fread(&entries[i], RCMOD_DIRENTRY_SIZE_ON_DISK, 1, file) != 1) {
			free(entries);
			RA_mod_free(mod);
			return RA_FAILURE("can't read directory entries");
		}
	}
	
	for(u32 i = 0; i < mod->asset_count; i++) {
		mod->assets[i].toc.has_header = true;
		if(fseek(file, entries[i].offset, SEEK_SET) != 0) {
			free(entries);
			RA_mod_free(mod);
			return RA_FAILURE("can't seek to asset header");
		}
		if(fread(&mod->assets[i].toc.header, sizeof(RA_TocAssetHeader), 1, file) != 1) {
			free(entries);
			RA_mod_free(mod);
			return RA_FAILURE("can't read asset header");
		}
		mod->assets[i].toc.location.offset = entries[i].offset + sizeof(RA_TocAssetHeader);
		mod->assets[i].toc.location.size = entries[i].size - sizeof(RA_TocAssetHeader);
		mod->assets[i].toc.path_hash = entries[i].hash;
		mod->assets[i].toc.group = entries[i].group;
	}
	
	free(entries);
	
	if(snprintf(mod->archive_path, sizeof(mod->archive_path), "mods\\%s", mod_file_name) < 0) {
		RA_mod_free(mod);
		return RA_FAILURE("path too long");
	}
	
	return RA_SUCCESS;
}
