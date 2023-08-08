#include "util.h"

#include <ctype.h>
#include <stdarg.h>

RA_Result RA_failure(const char* format, ...) {
	va_list args;
	va_start(args, format);
	
	static char message[16 * 1024];
	vsnprintf(message, 16 * 1024, format, args);
	
	va_end(args);
	return message;
}

void RA_file_fix_path(char* path) {
	u64 path_size = strlen(path);
	for(u64 i = 0; i < path_size; i++) {
		if(path[i] == '\\') {
			path[i] = '/';
		} else {
			path[i] = tolower(path[i]);
		}
	}
}

RA_Result RA_file_read(u8** data_dest, u32* size_dest, const char* path) {
	FILE* file = fopen(path, "rb");
	if(file == NULL) {
		return "failed to open file for reading";
	}
	
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);
	if(size > 1024 * 1024 * 1024) {
		fclose(file);
		return "file too large";
	}
	*size_dest = (u32) size;
	
	u8* data = malloc(size);
	if(data == NULL) {
		fclose(file);
		return "failed to allocate memory for file contents";
	}
	if(fread(data, size, 1, file) != 1) {
		free(data);
		fclose(file);
		return "failed to read file";
	}
	*data_dest = data;
	
	fclose(file);
	
	return RA_SUCCESS;
}

RA_Result RA_file_write(const char* path, u8* data, u32 size) {
	FILE* file = fopen(path, "wb");
	if(file == NULL) {
		return "fopen";
	}
	if(fwrite(data, size, 1, file) != 1) {
		fclose(file);
		return "fwrite";
	}
	fclose(file);
	return RA_SUCCESS;
}
