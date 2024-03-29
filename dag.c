#include "libra/dat_container.h"
#include "libra/dependency_dag.h"

static void list(const char* input_file);
static void deps(const char* input_file);
static void lookup(const char* input_file, const char* hash_str);
static void rebuild(const char* input_file, const char* output_file);
static void print_help();

int main(int argc, char** argv) {
	if(argc == 3 && strcmp(argv[1], "list") == 0) {
		list(argv[2]);
	} else if(argc == 3 && strcmp(argv[1], "deps") == 0) {
		deps(argv[2]);
	} else if(argc == 4 && strcmp(argv[1], "lookup") == 0) {
		lookup(argv[2], argv[3]);
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
	s64 size;
	if((result = RA_file_read(input_file, &data, &size)) != RA_SUCCESS) {
		fprintf(stderr, "Failed to read input file '%s'.\n", input_file);
		exit(1);
	}
	
	RA_DependencyDag dag;
	if((result = RA_dag_parse(&dag, data, (u32) size)) != RA_SUCCESS) {
		fprintf(stderr, "Failed to parse DAG file '%s' (%s).\n", input_file, result->message);
		exit(1);
	}
	
	for(u32 i = 0; i < dag.asset_count; i++) {
		RA_DependencyDagAsset* asset = &dag.assets[i];
		printf("%s\n", asset->name);
	}
	
	RA_dag_free(&dag, true);
}

static void deps(const char* input_file) {
	RA_Result result;
	
	u8* data;
	s64 size;
	if((result = RA_file_read(input_file, &data, &size)) != RA_SUCCESS) {
		fprintf(stderr, "Failed to read input file '%s'.\n", input_file);
		exit(1);
	}
	
	RA_DependencyDag dag;
	if((result = RA_dag_parse(&dag, data, (u32) size)) != RA_SUCCESS) {
		fprintf(stderr, "Failed to parse DAG file '%s' (%s).\n", input_file, result->message);
		exit(1);
	}
	
	for(u32 i = 0; i < dag.asset_count; i++) {
		RA_DependencyDagAsset* asset = &dag.assets[i];
		printf("%s\n", asset->name);
		for(u32 j = 0; j < asset->dependency_count; j++) {
			RA_DependencyDagAsset* dependency = &dag.assets[asset->dependencies[j]];
			printf("\t%s\n", dependency->name);
		}
	}
	
	RA_dag_free(&dag, true);
}

static void lookup(const char* input_file, const char* hash_str) {
	RA_Result result;
	
	u8* data;
	s64 size;
	if((result = RA_file_read(input_file, &data, &size)) != RA_SUCCESS) {
		fprintf(stderr, "Failed to read input file '%s'.\n", input_file);
		exit(1);
	}
	
	RA_DependencyDag dag;
	if((result = RA_dag_parse(&dag, data, (u32) size)) != RA_SUCCESS) {
		fprintf(stderr, "Failed to parse DAG file '%s' (%s).\n", input_file, result->message);
		exit(1);
	}
	
	u64 hash = strtoull(hash_str, NULL, 16);
	RA_DependencyDagAsset* asset = RA_dag_lookup_asset(&dag, hash);
	if(asset == NULL) {
		fprintf(stderr, "No asset with that hash.\n");
		exit(1);
	}
	printf("%s\n", asset->name);
	
	RA_dag_free(&dag, true);
}

static void rebuild(const char* input_file, const char* output_file) {
	RA_Result result;
	
	u8* in_data;
	s64 in_size;
	if((result = RA_file_read(input_file, &in_data, &in_size)) != RA_SUCCESS) {
		fprintf(stderr, "Failed to read input file '%s' (%s).\n", input_file, result->message);
		exit(1);
	}
	
	RA_DependencyDag dag;
	if((result = RA_dag_parse(&dag, in_data, (u32) in_size)) != RA_SUCCESS) {
		fprintf(stderr, "Failed to parse DAG file '%s' (%s).\n", input_file, result->message);
		exit(1);
	}
	
	u8* out_data;
	s64 out_size;
	if((result = RA_dag_build(&dag, &out_data, &out_size)) != RA_SUCCESS) {
		fprintf(stderr, "Failed to rebuild DAG file '%s' (%s).\n", output_file, result->message);
		exit(1);
	}
	
	if((result = RA_file_write(output_file, out_data, out_size)) != RA_SUCCESS) {
		fprintf(stderr, "Failed to write output file '%s' (%s).\n", input_file, result->message);
		exit(1);
	}
	
	RA_dag_free(&dag, true);
}

static void print_help() {
	puts("A utility for working with Insomniac Games Dependency DAG files, such as those used by the PC version of Rift Apart.");
	puts("");
	puts("Commands:");
	puts("  list <input file> -- List all asset file paths, one per line.");
	puts("  deps <input file> -- List all asset file paths, and their dependencies.");
	puts("  lookup <input file> <hash> -- Lookup an asset by its hash.");
}
