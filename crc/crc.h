#ifndef CRC_H
#define CRC_H

unsigned int RA_crc_update(const unsigned char* data, long long size);
unsigned int RA_crc_string(const char* string);

#endif
