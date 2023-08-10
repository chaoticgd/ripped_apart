#ifndef CRC_H
#define CRC_H

#include <stdint.h>

uint32_t RA_crc_update(const unsigned char* data, long long size);
uint32_t RA_crc_string(const char* string);

uint64_t RA_crc64_path(const char* string);

#endif
