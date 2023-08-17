#ifndef CRC_H
#define CRC_H

#include <stdint.h>

#define CRC32_POLYNOMIAL 0xedb88320

uint32_t RA_crc32_update(uint32_t crc, const unsigned char* data, long long size);
uint32_t RA_crc32_string(const char* string);

uint64_t RA_crc64_path(const char* string);

#endif
