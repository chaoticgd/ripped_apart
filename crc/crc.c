#include "crc.h"

static int crc_initialised = 0;
static unsigned int crc32_table[256];

static void crc_init() {
	unsigned int polynomial = 0xedb88320;
	for(unsigned int i = 0; i < 256; i++) {
		unsigned int accumulator = i;
		for(int j = 0; j < 8; j++) {
			if(accumulator & 1) {
				accumulator = polynomial ^ (accumulator >> 1);
			} else {
				accumulator >>= 1;
			}
		}
		crc32_table[i] = accumulator;
	}
	crc_initialised = 1;
}

unsigned int RA_crc_update(const unsigned char* data, long long size) {
	if(!crc_initialised) {
		crc_init();
	}
	unsigned int crc = 0xedb88320;
	for(long long i = 0; i < size; i++) {
		crc = crc32_table[(crc ^ data[i]) & 0xff] ^ (crc >> 8);
	}
	return crc;
}

unsigned int RA_crc_string(const char* string) {
	return RA_crc_update((const unsigned char*) string, strlen(string));
}
