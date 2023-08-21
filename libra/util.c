#include "util.h"

#include <ctype.h>
#include <stdarg.h>

#include "platform.h"

RA_Result RA_failure(int line, const char* format, ...) {
	va_list args;
	va_start(args, format);
	
	static char message[16 * 1024];
	vsnprintf(message, 16 * 1024, format, args);
	
	// Copy it just in case one of the variadic arguments is a pointer to the
	// last error message.
	static char message_copy[16 * 1024];
	RA_string_copy(message_copy, message, sizeof(message_copy));
	
	static RA_Error error;
	memset(&error, 0, sizeof(RA_Error));
	error.message = message_copy;
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

RA_Result RA_file_read(const char* path, u8** data_dest, s64* size_dest) {
	FILE* file = fopen(path, "rb");
	if(file == NULL) {
		return RA_FAILURE("failed to open file for reading");
	}
	
	s64 size = RA_file_size(file);
	*size_dest = size;
	
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

RA_Result RA_file_write(const char* path, u8* data, s64 size) {
	FILE* file = fopen(path, "wb");
	if(file == NULL) {
		return RA_FAILURE("fopen");
	}
	if(size == 0 || fwrite(data, size, 1, file) != 1) {
		fclose(file);
		return RA_FAILURE("fwrite");
	}
	fclose(file);
	printf("File written: %s\n", path);
	return RA_SUCCESS;
}

s64 RA_file_size(FILE* file) {
	long old_offset = ftell(file);
	if(old_offset == -1) {
		return -1;
	}
	if(fseek(file, 0, SEEK_END) != 0) {
		return -1;
	}
	long size = ftell(file);
	if(size == -1) {
		return size;
	}
	if(fseek(file, old_offset, SEEK_SET) != 0) {
		return -1;
	}
	return (s64) size;
}

void RA_remove_file_name(char* dest, s64 buffer_size, const char* src) {
	RA_string_copy(dest, src, buffer_size);
	char* seperator_forward = strrchr(dest, '/');
	char* seperator_backward = strrchr(dest, '\\');
	if(seperator_forward && seperator_forward > seperator_backward) {
		*seperator_forward = '\0';
	} else if(seperator_backward) {
		*seperator_backward = '\0';
	}
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
			RA_make_dir(dir_path);
			*ptr = seperator;
		}
	}
	RA_make_dir(dir_path);
	
	return RA_SUCCESS;
}

void RA_string_copy(char* dest, const char* src, s64 buffer_size) {
	for(s64 i = 0; i < buffer_size; i++) {
		dest[i] = *src;
		if(*src == '\0') break;
		src++;
	}
	dest[buffer_size - 1] = '\0';
}

const char* RA_string_find_substring_no_case(const char* haystack, const char* needle) {
	s64 haystack_size = strlen(haystack);
	s64 needle_size = strlen(needle);
	for(s64 i = 0; i < haystack_size - needle_size + 1; i++) {
		s64 j;
		for(j = 0; j < needle_size; j++) {
			if(toupper(needle[j]) != toupper(haystack[i + j])) {
				break;
			}
		}
		if(j == needle_size) {
			return &haystack[i];
		}
	}
	return NULL;
}

void RA_crc_string_parse(RA_CRCString* crc_string, u8* file_data, u32 file_size) {
	if(crc_string->on_disk.string_offset < file_size) {
		crc_string->string = (char*) file_data + crc_string->on_disk.string_offset;
	} else {
		crc_string->string = NULL;
	}
}
