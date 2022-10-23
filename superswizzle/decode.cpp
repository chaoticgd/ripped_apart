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
	for(int32_t y = 0; y < bheight; y++) {
		memcpy(&dest[(y * bwidth) * 4], &src[((origin_y + y) * iwidth + origin_x) * 4], bwidth * 4);
	}
}

static void set_block(uint8_t* dest, uint8_t* src, int32_t bx, int32_t by, int32_t bwidth, int32_t bheight, int32_t iwidth, int32_t iheight) {
	int32_t origin_x = bx * bwidth;
	int32_t origin_y = by * bheight;
	for(int32_t y = 0; y < bheight; y++) {
		memcpy(&dest[((origin_y + y) * iwidth + origin_x) * 4], &src[(y * bwidth) * 4], bwidth * 4);
	}
}

extern "C" {

void decode_init() {
	rgbcx::init(rgbcx::bc1_approx_mode::cBC1Ideal);
}

void decode_bc1(uint8_t* dest, uint8_t* src, int32_t width, int32_t height) {
	int32_t blocks_x = width / 4;
	int32_t blocks_y = height / 4;
	
	for(int32_t y = 0; y < blocks_y; y++) {
		for(int32_t x = 0; x < blocks_x; x++) {
			uint8_t unpacked[64];
			rgbcx::unpack_bc1(&src[(y * blocks_x + x) * 8], unpacked, true, rgbcx::bc1_approx_mode::cBC1Ideal);
			
			set_block(dest, unpacked, x, y, 4, 4, width, height);
		}
	}
}

void decode_bc7(uint8_t* dest, uint8_t* src, int32_t width, int32_t height) {
	
}

void unswizzle(uint8_t* dest, uint8_t* src, int32_t width, int32_t height, const int* swizzle_table) {
	for(int32_t megay = 0; megay < height; megay += 256) {
		for(int32_t megax = 0; megax < width; megax += 256) {
			for(int32_t blocky = 0; blocky < 64; blocky++) {
				for(int32_t blockx = 0; blockx < 64; blockx++) {
					int32_t dest_index = blocky * 64 + blockx;
					int32_t src_index = swizzle_table[dest_index] - 1;
					int32_t srcblockx = src_index % 64;
					int32_t srcblocky = src_index / 64;
					
					int32_t srcx = megax / 4 + srcblockx;
					int32_t srcy = megay / 4 + srcblocky;
					
					int32_t destx = megax / 4 + blockx;
					int32_t desty = megay / 4 + blocky;
					
					uint8_t block[64];
					get_block(block, src, srcx, srcy, 4, 4, width, height);
					set_block(dest, block, destx, desty, 4, 4, width, height);
				}
			}
		}
	}
}

}
