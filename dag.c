#include "libra/dat_container.h"
#include "libra/dependency_dag.h"

static void ls(const char* input_file);
static void print_help();

int main(int argc, char** argv) {
	RA_dat_init();
	if(argc == 3 && strcmp(argv[1], "list") == 0) {
		ls(argv[2]);
	} else {
		print_help();
		return 1;
	}
}

static void ls(const char* input_file) {
	RA_Result result;
	
	u8* data;
	u32 size;
	if((result = RA_file_read(&data, &size, input_file)) != RA_SUCCESS) {
		fprintf(stderr, "Failed to read input file '%s'.\n", input_file);
		exit(1);
	}
	
	RA_DependencyDag dag;
	if((result = RA_dag_parse(&dag, data, size)) != RA_SUCCESS) {
		fprintf(stderr, "Failed to parse DAG file '%s' (%s).", input_file, result);
		exit(1);
	}
	
	for(u32 i = 0; i < dag.asset_count; i++) {
		RA_DependencyDagAsset* asset = &dag.assets[i];
		printf("%s\n", asset->path);
	}
	
	RA_dag_free(&dag, true);
}

static void print_help() {
	puts("A utility for working with Insomniac Games Dependency DAG files, such as those used by the PC version of Rift Apart.");
	puts("");
	puts("Commands:");
	puts("  list <input file>");
}
