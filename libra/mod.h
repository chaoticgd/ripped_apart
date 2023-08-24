#ifndef LIBRA_MOD_H
#define LIBRA_MOD_H

#include "util.h"
#include "table_of_contents.h"

typedef enum {
	RA_MOD_FORMAT_STAGE,
	RA_MOD_FORMAT_RCMOD
} RA_ModFormat;

typedef struct {
	b8 enabled;
	char file_name[RA_MAX_PATH];
	RA_ModFormat format;
	char* name;
	char* version;
	char* description;
	char* author;
} RA_Mod;

typedef void (ModLoadErrorFunc)(const char* file_name, RA_Result result);
RA_Result RA_mod_list_load(RA_Mod** mods_dest, u32* mod_count_dest, const char* game_dir, ModLoadErrorFunc* error_func);
void RA_mod_list_free(RA_Mod* mods, u32 mod_count);

RA_Result RA_install_mods(RA_Mod* mods, u32 mod_count, RA_TableOfContents* toc, u32* success_count_dest, u32* fail_count_dest, const char* game_dir, ModLoadErrorFunc* error_func);

#endif
