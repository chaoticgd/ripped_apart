#include "libra/mod.h"
#include "libra/archive.h"
#include "libra/platform.h"
#include "libra/dependency_dag.h"
#include "libra/table_of_contents.h"

static void print_help();

static void report_mod_load_error(const char* file_name, RA_Result result) {
	if(result != RA_SUCCESS) {
		fprintf(stderr, "error: Failed to load mod '%s' (%s). The table of contents has not been modified.\n", file_name, result->message);
	}
}

static void report_mod_install_error(const char* file_name, RA_Result result) {
	if(result != RA_SUCCESS) {
		fprintf(stderr, "error: Failed to install mod '%s' (%s).\b", file_name, result->message);
	}
}

int main(int argc, char** argv) {
	RA_Result result;
	
	if(argc != 2) {
		print_help();
		return 1;
	}
	
	const char* game_dir = argv[1];
	
	char toc_path[RA_MAX_PATH];
	snprintf(toc_path, RA_MAX_PATH, "%s/toc", game_dir);
	char toc_backup_path[RA_MAX_PATH];
	snprintf(toc_backup_path, RA_MAX_PATH, "%s/toc.bak", game_dir);
	
	if(!RA_file_exists(toc_backup_path)) {
		u8* backup_data;
		s64 backup_size;
		if((result = RA_file_read(toc_path, &backup_data, &backup_size)) != RA_SUCCESS) {
			fprintf(stderr, "error: Failed to read toc file (%s).\n", result->message);
			return 1;
		}
		
		if((result = RA_file_write(toc_backup_path, backup_data, backup_size)) != RA_SUCCESS) {
			fprintf(stderr, "error: Failed to write backup toc file (%s). The table of contents has not been modified.\n", result->message);
			return 1;
		}
		
		RA_free(backup_data);
	}
	
	u8* in_data;
	s64 in_size;
	if((result = RA_file_read(toc_backup_path, &in_data, &in_size)) != RA_SUCCESS) {
		fprintf(stderr, "error: Failed to read toc file (%s).\n", result->message);
		return 1;
	}
	
	RA_TableOfContents toc;
	if((RA_toc_parse(&toc, in_data, (u32) in_size)) != RA_SUCCESS) {
		fprintf(stderr, "error: Failed to parse toc file (%s). "
			"Try restoring to the toc.bak file, or delete the toc file and validate files in Steam.\n",
			result->message);
		return 1;
	}
	
	RA_Mod* mods;
	u32 mod_count;
	if((result = RA_mod_list_load(&mods, &mod_count, game_dir, report_mod_load_error)) != RA_SUCCESS) {
		fprintf(stderr, "error: Failed to load mods (%s). The table of contents has not been modified.\n", result->message);
		return 1;
	}
	
	for(u32 i = 0; i < mod_count; i++) {
		mods[i].enabled = true;
		printf("Loaded mod: %s\n", mods[i].file_name);
	}
	
	u32 success_count;
	u32 fail_count;
	if((result = RA_install_mods(mods, mod_count, &toc, &success_count, &fail_count, game_dir, report_mod_install_error)) != RA_SUCCESS) {
		fprintf(stderr, "error: Failed to install mods (%s). The table of contents has not been modified.\n", result->message);
		return 1;
	}
	
	u8* out_data;
	s64 out_size;
	if((result = RA_toc_build(&toc, &out_data, &out_size)) != RA_SUCCESS) {
		fprintf(stderr, "error: Failed to build toc (%s). The table of contents has not been modified.\n", result->message);
		return 1;
	}
	
	if((result = RA_file_write(toc_path, out_data, out_size)) != RA_SUCCESS) {
		fprintf(stderr, "error: Failed to write toc file (%s).\n", result->message);
		return 1;
	}
	
	printf("%u mods installed successfully, %u mods failed to install.\n", success_count, fail_count);
	
	RA_free(out_data);
	RA_mod_list_free(mods, mod_count);
	RA_toc_free(&toc, FREE_FILE_DATA);
}

static void print_help() {
	puts("rebuildtoc -- part of https://github.com/chaoticgd/ripped_apart");
	puts("  Rebuild the toc (table of contents) file based on the contents of the mod directory.");
	puts("");
	puts("Usage: ./rebuildtoc <game directory>");
}
