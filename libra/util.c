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

static int alloc_dummy;

void* RA_malloc(size_t size) {
	if(size == 0) {
		return (void*) &alloc_dummy;
	}
	return malloc(size);
}

void RA_free(void* ptr) {
	if(ptr != (void*) &alloc_dummy) {
		free(ptr);
	}
}

void* RA_calloc(size_t nmemb, size_t size) {
	if(nmemb == 0 || size == 0) {
		return &alloc_dummy;
	}
	return calloc(nmemb, size);
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
	
	u8* data = RA_malloc(size + 1);
	if(data == NULL) {
		fclose(file);
		return RA_FAILURE("failed to allocate memory for file contents");
	}
	if(fread(data, size, 1, file) != 1) {
		RA_free(data);
		fclose(file);
		return RA_FAILURE("failed to read file");
	}
	data[size] = '\0';
	*data_dest = data;
	
	fclose(file);
	
	return RA_SUCCESS;
}

RA_Result RA_file_write(const char* path, u8* data, s64 size) {
	FILE* file = fopen(path, "wb");
	if(file == NULL) {
		return RA_FAILURE("fopen");
	}
	if(size != 0 && fwrite(data, size, 1, file) != 1) {
		fclose(file);
		return RA_FAILURE("fwrite");
	}
	fclose(file);
	printf("File written: %s\n", path);
	return RA_SUCCESS;
}

s64 RA_file_size(FILE* file) {
	s64 old_offset = ftell(file);
	if(old_offset == -1) {
		return -1;
	}
	if(fseek(file, 0, SEEK_END) != 0) {
		return -1;
	}
	s64 size = ftell(file);
	if(size == -1) {
		return size;
	}
	if(fseek(file, old_offset, SEEK_SET) != 0) {
		return -1;
	}
	return size;
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

static s32 to_upper(s32 c) {
	if(c >= 'a' && c <= 'z') {
		return (c + 'A') - 'a';
	} else {
		return c;
	}
}

const char* RA_string_find_substring_no_case(const char* haystack, const char* needle) {
	s64 haystack_size = strlen(haystack);
	s64 needle_size = strlen(needle);
	for(s64 i = 0; i < haystack_size - needle_size + 1; i++) {
		s64 j;
		for(j = 0; j < needle_size; j++) {
			if(to_upper(needle[j]) != to_upper(haystack[i + j])) {
				break;
			}
		}
		if(j == needle_size) {
			return &haystack[i];
		}
	}
	return NULL;
}

s32 RA_string_compare_no_case(const char* lhs, const char* rhs) {
	for(;;) {
		char c1 = to_upper(*lhs);
		char c2 = to_upper(*rhs);
		if(c1 == '\0' && c2 == '\0') {
			break;
		} else if(c1 == c2) {
			lhs++;
			rhs++;
		} else {
			return (c1 > c2) ? 1 : -1;
		}
	}
	return 0;
}

void RA_crc_string_parse(RA_CRCString* crc_string, u8* file_data, u32 file_size) {
	if(crc_string->on_disk.string_offset < file_size) {
		crc_string->string = (char*) file_data + crc_string->on_disk.string_offset;
	} else {
		crc_string->string = NULL;
	}
}

RA_Result RA_diff_buffers(const u8* lhs, u32 lhs_size, const u8* rhs, u32 rhs_size, const char* context, b8 print_hex_dump_on_failure) {
	u32 min_size = MIN(lhs_size, rhs_size);
	u32 max_size = MAX(lhs_size, rhs_size);
	
	u32 diff_offset = UINT32_MAX;
	for(u32 i = 0; i < min_size; i++) {
		if(lhs[i] != rhs[i]) {
			diff_offset = i;
			break;
		}
	}
	
	if(diff_offset == UINT32_MAX) {
		if(lhs_size == rhs_size) {
			return RA_SUCCESS;
		} else {
			diff_offset = min_size;
		}
	}
	
	RA_Result error = RA_FAILURE("%s differs at offset %x", context, diff_offset);
	if(!print_hex_dump_on_failure) {
		return error;
	}
	
	printf("%s\n", error->message);
	
	s64 row_start = (diff_offset / 0x10) * 0x10;
	s64 hexdump_begin = MAX(0, row_start - 0x50);
	s64 hexdump_end = max_size;
	for(s64 i = hexdump_begin; i < hexdump_end; i += 0x10) {
		printf("%08x: ", (s32) i);
		const u8* current = lhs;
		u32 current_size = lhs_size;
		for(u32 j = 0; j < 2; j++) {
			for(s64 j = 0; j < 0x10; j++) {
				s64 pos = i + j;
				const char* colour = NULL;
				if(pos < lhs_size && pos < rhs_size) {
					if(lhs[pos] == rhs[pos]) {
						colour = "32";
					} else {
						colour = "31";
					}
				} else if(pos < current_size) {
					colour = "33";
				} else {
					printf("   ");
				}
				if(colour != NULL) {
					printf("\033[%sm%02x\033[0m ", colour, current[pos]);
				}
				if(j % 4 == 3 && j != 0xf) {
					printf(" ");
				}
			}
			printf("| ");
			current = rhs;
			current_size = rhs_size;
		}
		printf("\n");
	}
	printf("\n");
	
	return error;
}
