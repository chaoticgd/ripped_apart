#include "libra/dat_container.h"
#include "libra/dependency_dag.h"

static void list(const char* input_file);
static void deps(const char* input_file);
static void rebuild(const char* input_file, const char* output_file);
static void print_help();

int main(int argc, char** argv) {
	if(argc == 3 && strcmp(argv[1], "list") == 0) {
		list(argv[2]);
	} else if(argc == 3 && strcmp(argv[1], "deps") == 0) {
		deps(argv[2]);
	} else if(argc == 4 && strcmp(argv[1], "rebuild") == 0) {
		rebuild(argv[2], argv[3]);
	} else {
		print_help();
		return 1;
	}
}

static void list(const char* input_file) {
	RA_Result result;
	
	u8* data;
	u32 size;
	if((result = RA_file_read(&data, &size, input_file)) != RA_SUCCESS) {
		fprintf(stderr, "Failed to read input file '%s'.\n", input_file);
		exit(1);
	}
	
	RA_DependencyDag dag;
	if((result = RA_dag_parse(&dag, data, size)) != RA_SUCCESS) {
		fprintf(stderr, "Failed to parse DAG file '%s' (%s).\n", input_file, result);
		exit(1);
	}
	
	for(u32 i = 0; i < dag.asset_count; i++) {
		RA_DependencyDagAsset* asset = &dag.assets[i];
		printf("%s\n", asset->path);
	}
	
	RA_dag_free(&dag, true);
}

static void deps(const char* input_file) {
	RA_Result result;
	
	u8* data;
	u32 size;
	if((result = RA_file_read(&data, &size, input_file)) != RA_SUCCESS) {
		fprintf(stderr, "Failed to read input file '%s'.\n", input_file);
		exit(1);
	}
	
	RA_DependencyDag dag;
	if((result = RA_dag_parse(&dag, data, size)) != RA_SUCCESS) {
		fprintf(stderr, "Failed to parse DAG file '%s' (%s).\n", input_file, result);
		exit(1);
	}
	
	for(u32 i = 0; i < dag.asset_count; i++) {
		RA_DependencyDagAsset* asset = &dag.assets[i];
		printf("%s\n", asset->path);
		for(u32 j = 0; j < asset->dependency_count; j++) {
			RA_DependencyDagAsset* dependency = &dag.assets[asset->dependencies[j]];
			printf("\t%s\n", dependency->path);
		}
	}
	
	RA_dag_free(&dag, true);
}

static void rebuild(const char* input_file, const char* output_file) {
	RA_Result result;
	
	u8* in_data;
	u32 in_size;
	if((result = RA_file_read(&in_data, &in_size, input_file)) != RA_SUCCESS) {
		fprintf(stderr, "Failed to read input file '%s' (%s).\n", input_file, result);
		exit(1);
	}
	
	RA_DependencyDag dag;
	if((result = RA_dag_parse(&dag, in_data, in_size)) != RA_SUCCESS) {
		fprintf(stderr, "Failed to parse DAG file '%s' (%s).\n", input_file, result);
		exit(1);
	}
	
	u8* out_data;
	u32 out_size;
	if((result = RA_dag_build(&dag, &out_data, &out_size)) != RA_SUCCESS) {
		fprintf(stderr, "Failed to rebuild DAG file '%s' (%s).\n", output_file, result);
		exit(1);
	}
	
	if((result = RA_file_write(output_file, out_data, out_size)) != RA_SUCCESS) {
		fprintf(stderr, "Failed to write output file '%s' (%s).\n", input_file, result);
		exit(1);
	}
	
	RA_dag_free(&dag, true);
}

static void print_help() {
	puts("A utility for working with Insomniac Games Dependency DAG files, such as those used by the PC version of Rift Apart.");
	puts("");
	puts("Commands:");
	puts("  list <input file> --- List all asset file paths, one per line.");
	puts("  deps <input file> --- list all asset file paths, and their dependencies.");
}
