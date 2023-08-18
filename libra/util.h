#ifndef LIBRA_UTIL_H
#define LIBRA_UTIL_H

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "../crc/crc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

#define verify(cond, msg) if(!(cond)) { printf(msg "\n"); exit(1); }
#define check_fread(num) if((num) != 1) { printf("error: fread failed (line %d)\n", __LINE__); exit(1); }
#define check_fwrite(num) if((num) != 1) { printf("error: fwrite failed (line %d)\n", __LINE__); exit(1); }
#define check_fputc(num) if((num) == EOF) { printf("error: fputc failed (line %d)\n", __LINE__); exit(1); }
#define check_fputs(num) if((num) == EOF) { printf("error: fputs failed (line %d)\n", __LINE__); exit(1); }
#define check_fseek(num) if((num) != 0) { printf("error: fseek failed (line %d)\n", __LINE__); exit(1); }

typedef char s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef unsigned char u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef float f64;

typedef char b8;
#define false 0
#define true 1

typedef struct {
	const char* message;
	int line;
} RA_Error;
typedef RA_Error* RA_Result;
#define RA_SUCCESS NULL
#define RA_FAILURE(...) RA_failure(__LINE__, __VA_ARGS__)
RA_Result RA_failure(int line, const char* format, ...);

#define MIN(x, y) (((y) < (x)) ? (y) : (x))
#define MAX(x, y) (((y) > (x)) ? (y) : (x))
#define ALIGN(value, alignment) ((value) + (-(value) & ((alignment) - 1)))
#define FOURCC(string) ((string)[0] | (string)[1] << 8 | (string)[2] << 16 | (string)[3] << 24)

#define RA_PI 3.14159265358979323846

void RA_file_fix_path(char* path);
RA_Result RA_file_read(u8** data_dest, u32* size_dest, const char* path);
RA_Result RA_file_write(const char* path, u8* data, u32 size);
RA_Result RA_make_dirs(const char* file_path);

#define RA_MAX_PATH 1024
// On Windows long is only 4 bytes, so the regular libc standard IO functions
// are crippled, hence we need to use these special versions instead.
#ifdef _MSC_VER
	#define fseek _fseeki64
	#define ftell _ftelli64
#endif

typedef enum {
	DONT_FREE_FILE_DATA = 0,
	FREE_FILE_DATA = 1
} ShouldFreeFileData;

void RA_string_copy(char* dest, const char* src, s64 buffer_size);

typedef union {
	const char* string;
	struct {
		u32 crc;
		u32 string_offset;
	} on_disk;
} RA_CRCString;

void RA_crc_string_parse(RA_CRCString* crc_string, u8* file_data, u32 file_size);

#ifdef __GNUC__
	#define __maybe_unused  __attribute__((unused))
#else
	#define __maybe_unused
#endif
#define RA_ASSERT_SIZE(type, size) __maybe_unused static char assert_size_ ##type[(sizeof(type) == size) ? 1 : -1]

#ifdef __cplusplus
}
#endif

#endif
