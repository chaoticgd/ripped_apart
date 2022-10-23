#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>
#include <stdio.h>

#define RGBCX_IMPLEMENTATION
#include "../thirdparty/bc7enc/rgbcx.h"

static void get_block(uint8_t* dest, uint8_t* src, int32_t bx, int32_t by, int32_t bwidth, int32_t bheight, int32_t iwidth, int32_t iheight) {
	int32_t origin_x = bx * bwidth;
	int32_t origin_y = by * bheight;
	for(int32_t y = 0; y < bheight; y++) {//printf("read %x <- %x\n", (y * bwidth) * 4, ((origin_y + y) * iwidth + origin_x) * 4);
	printf("access %x slice %x bufsize %x\n", ((origin_y + y) * iwidth + origin_x) * 4, bwidth * 4, iwidth * iheight * 4);
		memcpy(&dest[(y * bwidth) * 4], &src[((origin_y + y) * iwidth + origin_x) * 4], bwidth * 4);
	}
}

static void set_block(uint8_t* dest, uint8_t* src, int32_t bx, int32_t by, int32_t bwidth, int32_t bheight, int32_t iwidth, int32_t iheight) {
	int32_t origin_x = bx * bwidth;
	int32_t origin_y = by * bheight;
	for(int32_t y = 0; y < bheight; y++) {//printf("write %x <- %x\n", ((origin_y + y) * iwidth + origin_x) * 4, (y * bwidth) * 4);
		memcpy(&dest[((origin_y + y) * iwidth + origin_x) * 4], &src[(y * bwidth) * 4], bwidth * 4);
	}
}

extern "C" {

void decode_init() {
	rgbcx::init(rgbcx::bc1_approx_mode::cBC1Ideal);
	
	//const char* ptr = BC1_TABLE;
	//while(*ptr) {
	//	int num;
	//	sscanf(ptr, "%08d", &num);
	//	printf("%d, ", num);
	//	ptr += 8;
	//}
}

void decode_bc1(uint8_t* dest, uint8_t* src, int32_t width, int32_t height) {
	int32_t blocks_x = width / 4;
	int32_t blocks_y = height / 4;
	
	for(int32_t y = 0; y < blocks_y; y++) {
		for(int32_t x = 0; x < blocks_x; x++) {
			uint8_t block[8];
			memcpy(block, &src[(y * blocks_x + x) * 8], 8);
			
			uint8_t unpacked[64];
			rgbcx::unpack_bc1(&src[(y * blocks_x + x) * 8], unpacked, true, rgbcx::bc1_approx_mode::cBC1Ideal);
			
			set_block(dest, unpacked, x, y, 4, 4, width, height);
		}
	}
}

void unswizzle_bc1(uint8_t* dest, uint8_t* src, int32_t width, int32_t height) {
	int32_t blocks_x = width / 8;
	int32_t blocks_y = height / 4;
	for(int32_t y = 0; y < blocks_y; y++) {
		for(int32_t x = 0; x < blocks_x; x++) {
			int32_t origin_x = x * 8;
		int32_t origin_y = y * 4;
			printf("block %x %x\n", origin_x, origin_y);
			
			uint8_t inblock[4096 * 4];
			get_block(inblock, src, x, y, 8, 4, width, height);
			
			//uint8_t outblock[4096 * 4];
			//for(int32_t i = 0; i < 4096; i++) {
			//	*(uint32_t*) &outblock[i * 4] = *(uint32_t*) &inblock[(BC1_TAB[i] - 1) * 4];
			//}
			
			set_block(dest, inblock, x, y, 8, 4, width, height);
		}
	}
}

void decode_bc7(uint8_t* dest, uint8_t* src, int32_t width, int32_t height) {
	
}

}
