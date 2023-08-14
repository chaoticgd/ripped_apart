#include "util.h"

#include <ctype.h>
#include <stdarg.h>

#ifdef WIN32
	#define mkdir_portable(path) _mkdir(path)
#else
	#include <sys/stat.h>
	#define mkdir_portable(path) mkdir(path, 0777)
#endif

RA_Result RA_failure(int line, const char* format, ...) {
	va_list args;
	va_start(args, format);
	
	static char message[16 * 1024];
	vsnprintf(message, 16 * 1024, format, args);
	
	static RA_Error error;
	memset(&error, 0, sizeof(RA_Error));
	error.message = message;
	error.line = line;
	
	va_end(args);
	return &error;
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
		return RA_FAILURE("failed to open file for reading");
	}
	
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);
	if(size > 1024 * 1024 * 1024) {
		fclose(file);
		return RA_FAILURE("file too large");
	}
	*size_dest = (u32) size;
	
	u8* data = malloc(size);
	if(data == NULL) {
		fclose(file);
		return RA_FAILURE("failed to allocate memory for file contents");
	}
	if(fread(data, size, 1, file) != 1) {
		free(data);
		fclose(file);
		return RA_FAILURE("failed to read file");
	}
	*data_dest = data;
	
	fclose(file);
	
	return RA_SUCCESS;
}

RA_Result RA_file_write(const char* path, u8* data, u32 size) {
	FILE* file = fopen(path, "wb");
	if(file == NULL) {
		return RA_FAILURE("fopen");
	}
	if(fwrite(data, size, 1, file) != 1) {
		fclose(file);
		return RA_FAILURE("fwrite");
	}
	fclose(file);
	printf("File written: %s\n", path);
	return RA_SUCCESS;
}

RA_Result RA_make_dirs(const char* file_path) {
	// Remove filename.
	char dir_path[RA_MAX_PATH];
	RA_string_copy(dir_path, file_path, RA_MAX_PATH);
	char* seperator_forward = strrchr(dir_path, '/');
	char* seperator_backward = strrchr(dir_path, '\\');
	if(seperator_forward && seperator_forward > seperator_backward) {
		*seperator_forward = '\0';
	} else if(seperator_backward) {
		*seperator_backward = '\0';
	}
	
	// Make the directories.
	for(char* ptr = dir_path + 1; *ptr != '\0'; ptr++) {
		if(*ptr == '/' || *ptr == '\\') {
			char seperator = *ptr;
			*ptr = '\0';
			mkdir_portable(dir_path);
			*ptr = seperator;
		}
	}
	mkdir_portable(dir_path);
	
	return RA_SUCCESS;
}

void RA_string_copy(char* dest, const char* src, s64 buffer_size) {
	for(s64 i = 0; i < buffer_size; i++) {
		*dest++ = *src;
		if(*src == '\0') break;
		src++;
	}
	dest[buffer_size - 1] = '\0';
}

void RA_crc_string_parse(RA_CRCString* crc_string, u8* file_data, u32 file_size) {
	if(crc_string->on_disk.string_offset < file_size) {
		crc_string->string = (char*) file_data + crc_string->on_disk.string_offset;
	} else {
		crc_string->string = NULL;
	}
}
