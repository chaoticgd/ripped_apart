#include <stdio.h>
#include <string.h>

#include "crc/crc.h"

int main(int argc, char** argv) {
	if(argc != 1 && argc != 2) {
		fprintf(stderr, "usage: ./genlumptypes [lump_types.txt]");
		return 1;
	}
	
	const char* in_path;
	if(argc == 1) {
		in_path = "libra/lump_types.txt";
	} else {
		in_path = argv[1];
	}
	FILE* in_file = fopen(in_path, "r");
	if(!in_file) {
		fprintf(stderr, "Failed to open input file.\n");
		return 1;
	}
	
	char out_path[256];
	strncpy(out_path, in_path, 256);
	char* extension = strrchr(out_path, '.');
	if(extension == NULL || extension[1] == '\0') {
		fprintf(stderr, "Failed to parse input path.\n");
	}
	extension[1] = 'h';
	extension[2] = '\0';
	
	FILE* out_file = fopen(out_path, "w");
	
	fprintf(out_file, "// This file is generated from lump_types.txt by genlumptypes.\n");
	
	char line[1024];
	while(fgets(line, 1024, in_file)) {
		if(strstr(line, "//") == line) {
			// Comment.
			continue;
		}
		
		char macro[128];
		char identifier[128];
		char name[128];
		sscanf(line, "%127s %127s %[^\n]\n", macro, identifier, name);
		
		if(strcmp(macro, "LUMP_TYPE") == 0) {
			fprintf(out_file, "LUMP_TYPE(%s, 0x%x, \"%s\")\n", identifier, RA_crc_string(name), name);
		} else if(strcmp(macro, "LUMP_TYPE_FAKE_NAME") == 0) {
			fprintf(out_file, "LUMP_TYPE(%s, %s, \"%s\")\n", identifier, name, identifier);
		} else {
			fprintf(stderr, "Failed to parse lump types file.\n");
			return 1;
		}
	}
}
