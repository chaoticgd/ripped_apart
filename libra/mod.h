#ifndef LIBRA_MOD_H
#define LIBRA_MOD_H

#include "util.h"
#include "table_of_contents.h"

typedef struct {
	RA_TocAsset toc;
} RA_ModAsset;

typedef struct {
	b8 initialised;
	b8 enabled;
	char file_name[RA_MAX_PATH];
	char archive_path[0x42];
	RA_ModAsset* assets;
	u32 asset_count;
	char* name;
	char* version;
	char* description;
	char* author;
} RA_Mod;

RA_Result RA_mod_list_load(RA_Mod** mods_dest, u32* mod_count_dest, const char* game_dir);
RA_Result RA_mod_list_rebuild_toc(RA_Mod* mods, u32 mod_count, RA_TableOfContents* toc);
void RA_mod_list_free(RA_Mod* mods, u32 mod_count);

RA_Result RA_mod_read(RA_Mod* mod, const char* game_dir, const char* mod_file_name);
void RA_mod_free(RA_Mod* mod);

#endif
