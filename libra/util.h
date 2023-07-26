#ifndef LIBRA_UTIL_H
#define LIBRA_UTIL_H

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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

#define X 0
#define Y 1
#define Z 2
#define W 3

typedef const char* RA_Result;
#define RA_SUCCESS NULL

#define MAX(x, y) (((y) > (x)) ? (y) : (x))

RA_Result RA_read_entire_file(u8** data_dest, u32* size_dest, const char* path);

void RA_crc_init();
u32 RA_crc_update(const u8* data, s64 size);
u32 RA_crc_string(const char* string);

#ifdef __cplusplus
}
#endif

#endif
