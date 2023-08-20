#include "../libra/util.h"
#include "../libra/dat_container.h"
#include "../libra/dependency_dag.h"
#include "../libra/table_of_contents.h"
#include "../libra/material.h"

#include <dirent.h>

static RA_Result test_file(const char* path);
static RA_Result test_dat_file(u8* data, u32 size);
static RA_Result test_toc_file(u8* data, u32 size);
static RA_Result test_dag_file(u8* data, u32 size);
static RA_Result test_material_file(RA_DatFile* dat);
static RA_Result test_toc_lookup_asset();

int main(int argc, const char** argv) {
	RA_Result result;
	
	const char* directory_path;
	if(argc == 2) {
		directory_path = argv[1];
	} else if(argc == 0 || argc == 1) {
		directory_path = "testdata";
	} else {
		fprintf(stderr, "usage: ./bin/test [dir]\n");
		return 1;
	}
	
	DIR* directory = opendir(directory_path);
	if(!directory) {
		fprintf(stderr, "error: Failed to enumerate testdata directory.\n");
		return 2;
	}
	
	struct dirent* entry;
	while((entry = readdir(directory)) != NULL) {
		if(entry->d_type == DT_REG) {
			char file_path[1024];
			snprintf(file_path, 1024, "%s/%s", directory_path, entry->d_name);
			
			if((result = test_file(file_path)) == RA_SUCCESS) {
				printf("success\n");
			} else {
				printf("%s\n", result->message);
			}
		}
	}
	
	printf("RA_toc_lookup_asset: ");
	if((result = test_toc_lookup_asset()) == RA_SUCCESS) {
		printf("success\n");
	} else {
		printf("%s\n", result->message);
	}
}

static RA_Result test_file(const char* path) {
	RA_Result result;
	
	printf("%s: ", path);
	u8* data;
	u32 size;
	RA_file_read(path, &data, &size);
	
	if(size >= 4 && *(u32*) data == FOURCC("1TAD")) {
		if((result = test_dat_file(data, size)) != RA_SUCCESS) {
			return result;
		}
	} else if(size >= 0xc && *(u32*) (data + 0x8) == FOURCC("1TAD")) {
		if((result = test_toc_file(data, size)) != RA_SUCCESS) {
			return result;
		}
	} else if(size >= 0x10 && *(u32*) (data + 0xc) == FOURCC("1TAD")) {
		if((result = test_dag_file(data, size)) != RA_SUCCESS) {
			return result;
		}
	} else {
		printf("skipped\n");
	}
	
	return RA_SUCCESS;
}

static RA_Result test_dat_file(u8* data, u32 size) {
	RA_Result result;
	
	RA_DatFile dat;
	RA_dat_parse(&dat, data, size, 0);
	
	switch(dat.asset_type_crc) {
		case RA_ASSET_TYPE_MATERIAL: {
			if((result = test_material_file(&dat)) != RA_SUCCESS) {
				return result;
			}
			break;
		}
		default: {
			printf("skipped\n");
		}
	}
	
	return RA_SUCCESS;
}

static RA_Result test_toc_file(u8* data, u32 size) {
	RA_Result result;
	
	RA_TableOfContents toc;
	if((result = RA_toc_parse(&toc, data, size)) != RA_SUCCESS) {
		return result;
	}
	
	u8* out_data;
	u32 out_size;
	if((result = RA_toc_build(&toc, &out_data, &out_size)) != RA_SUCCESS) {
		return result;
	}
	
	if((result = RA_dat_test(data + 0x8, size - 0x8, out_data + 0x8, out_size - 0x8, true)) != RA_SUCCESS) {
		RA_file_write("/tmp/test_toc", out_data, out_size);
		return result;
	}
	
	return RA_SUCCESS;
}

static RA_Result test_dag_file(u8* data, u32 size) {
	RA_Result result;
	
	RA_DependencyDag dag;
	if((result = RA_dag_parse(&dag, data, size)) != RA_SUCCESS) {
		return result;
	}
	
	u8* out_data;
	u32 out_size;
	if((result = RA_dag_build(&dag, &out_data, &out_size)) != RA_SUCCESS) {
		return result;
	}
	
	if((result = RA_dat_test(data + 0xc, size - 0xc, out_data + 0xc, out_size - 0xc, true)) != RA_SUCCESS) {
		return result;
	}
	
	return RA_SUCCESS;
}

static RA_Result test_material_file(RA_DatFile* dat) {
	//RA_Material material;
	//RA_material_parse(&material, dat);
	printf("todo\n");
	return RA_SUCCESS;
}

static RA_Result test_toc_lookup_asset() {
	RA_TocAsset assets[5];
	assets[0].group = 0;
	assets[0].path_hash = 123;
	assets[1].group = 1;
	assets[1].path_hash = 1;
	assets[2].group = 1;
	assets[2].path_hash = 123;
	assets[3].group = 2;
	assets[3].path_hash = 1234;
	assets[4].group = 2;
	assets[4].path_hash = 12345;
	
	if(RA_toc_lookup_asset(assets, ARRAY_SIZE(assets), 321, 1) != NULL) {
		return RA_FAILURE("1");
	}
	
	if(RA_toc_lookup_asset(assets, ARRAY_SIZE(assets), 1, 1) != &assets[1]) {
		return RA_FAILURE("2");
	}
	
	if(RA_toc_lookup_asset(assets, ARRAY_SIZE(assets), 1234, 2) != &assets[3]) {
		return RA_FAILURE("3");
	}
	
	if(RA_toc_lookup_asset(assets, ARRAY_SIZE(assets), 12345, 2) != &assets[4]) {
		return RA_FAILURE("4");
	}
	
	return RA_SUCCESS;
}
