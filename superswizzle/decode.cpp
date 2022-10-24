#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>
#include <stdio.h>

#define RGBCX_IMPLEMENTATION
#include "../thirdparty/bc7enc/rgbcx.h"
#include "../thirdparty/bc7enc/bc7decomp.h"

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
	int32_t blocks_x = width / 4;
	int32_t blocks_y = height / 4;
	
	for(int32_t y = 0; y < blocks_y; y++) {
		for(int32_t x = 0; x < blocks_x; x++) {
			uint8_t unpacked[64];
			bc7decomp::unpack_bc7(&src[(y * blocks_x + x) * 16], (bc7decomp::color_rgba*) unpacked);
			
			set_block(dest, unpacked, x, y, 4, 4, width, height);
		}
	}
}

void unswizzle(uint8_t* dest, uint8_t* src, int32_t width, int32_t height, const int* swizzle_table) {
	for(int32_t mega_y = 0; mega_y < height; mega_y += 256) {
		for(int32_t mega_x = 0; mega_x < width; mega_x += 256) {
			for(int32_t block_y = 0; block_y < 64; block_y++) {
				for(int32_t block_x = 0; block_x < 64; block_x++) {
					int32_t dest_index = block_y * 64 + block_x;
					int32_t src_index = swizzle_table[dest_index] - 1;
					int32_t src_block_x = src_index % 64;
					int32_t src_block_y = src_index / 64;
					
					int32_t src_x = mega_x / 4 + src_block_x;
					int32_t src_y = mega_y / 4 + src_block_y;
					
					int32_t dest_x = mega_x / 4 + block_x;
					int32_t dest_y = mega_y / 4 + block_y;
					
					uint8_t block[64];
					get_block(block, src, src_x, src_y, 4, 4, width, height);
					set_block(dest, block, dest_x, dest_y, 4, 4, width, height);
				}
			}
		}
	}
}

}
