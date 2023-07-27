#include "../libra/util.h"
#include "../libra/dat_container.h"
#include "../libra/material.h"

#include <dirent.h>

static void test_file(const char* path);
static void test_material_file(RA_DatFile* dat);

int main(int argc, const char** argv) {
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
	struct dirent* entry;
	if(!directory) {
		fprintf(stderr, "error: Failed to enumerate testdata directory.\n");
		return 2;
	}
	while((entry = readdir(directory)) != NULL) {
		if(entry->d_type == DT_REG) {
			char file_path[1024];
			snprintf(file_path, 1024, "%s/%s", directory_path, entry->d_name);
			test_file(file_path);
		}
	}
}

static void test_file(const char* path) {
	printf("%s ", path);
	u8* data;
	u32 size;
	RA_file_read(&data, &size, path);
	
	RA_DatFile dat;
	RA_dat_parse(&dat, data, size);
	
	switch(dat.asset_type_crc) {
		case RA_ASSET_TYPE_MATERIAL: {
			test_material_file(&dat);
			break;
		}
		default: {
			printf("skipped\n");
		}
	}
}

static void test_material_file(RA_DatFile* dat) {
	//RA_Material material;
	//RA_material_parse(&material, dat);
	printf("todo\n");
}
