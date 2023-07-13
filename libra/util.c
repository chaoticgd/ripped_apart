#include "util.h"

static uint32_t crc32_table[256];

void RA_crc_init() {
	uint32_t polynomial = 0xedb88320;
	for (uint32_t i = 0; i < 256; i++)  {
		uint32_t accumulator = i;
		for(int32_t j = 0; j < 8; j++)  {
			if (accumulator & 1) {
				accumulator = polynomial ^ (accumulator >> 1);
			} else {
				accumulator >>= 1;
			}
		}
		crc32_table[i] = accumulator;
	}
}

uint32_t RA_crc_update(const uint8_t* data, int64_t size) {
	uint32_t crc = 0xedb88320;
	for (int64_t i = 0; i < size; i++) {
		crc = crc32_table[(crc ^ data[i]) & 0xff] ^ (crc >> 8);
	}
	return crc;
}
