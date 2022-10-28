#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>
#include <stdio.h>

#define RGBCX_IMPLEMENTATION
#include "../thirdparty/bc7enc/rgbcx.h"
#include "../thirdparty/bc7enc/bc7decomp.h"
#include "../thirdparty/bc7enc/lodepng.h"

static void copy_block(uint8_t* dest, int32_t dx, int32_t dy, uint8_t* src, int32_t sx, int32_t sy, int32_t block_width, int32_t block_height, int32_t image_width) {
	for(int32_t y = 0; y < block_height; y++) {
		memcpy(
			&dest[((dy * block_height + y) * image_width + dx * block_width) * 4],
			&src[((sy * block_height + y) * image_width + sx * block_width) * 4],
			block_width * 4);
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
	rgbcx::init(rgbcx::bc1_approx_mode::cBC1IdealRound4);
}

void decode_bc1(uint8_t* dest, uint8_t* src, int32_t width, int32_t height) {
	for(int32_t y = 0; y < height / 4; y++) {
		for(int32_t x = 0; x < width / 4; x++) {
			uint8_t block[64];
			rgbcx::unpack_bc1(&src[(y * (width / 4) + x) * 8], block, true, rgbcx::bc1_approx_mode::cBC1IdealRound4);
			set_block(dest, block, x, y, 4, 4, width, height);
		}
	}
}

void decode_bc4(uint8_t* dest, uint8_t* src, int32_t width, int32_t height) {
	for(int32_t y = 0; y < height / 4; y++) {
		for(int32_t x = 0; x < width / 4; x++) {
			uint8_t block[64];
			rgbcx::unpack_bc4(&src[(y * (width / 4) + x) * 8], block, 4);
			for(int32_t i = 0; i < 64; i += 4) {
				block[i + 1] = block[i];
				block[i + 2] = block[i];
				block[i + 3] = 0xff;
			}
			set_block(dest, block, x, y, 4, 4, width, height);
		}
	}
}

void decode_bc7(uint8_t* dest, uint8_t* src, int32_t width, int32_t height) {
	for(int32_t y = 0; y < height / 4; y++) {
		for(int32_t x = 0; x < width / 4; x++) {
			uint8_t block[64];
			bc7decomp::unpack_bc7(&src[(y * (width / 4) + x) * 16], (bc7decomp::color_rgba*) block);
			set_block(dest, block, x, y, 4, 4, width, height);
		}
	}
}

void unswizzle(uint8_t* dest, uint8_t* src, int32_t width, int32_t height, const int32_t* swizzle_table) {
	int32_t src_mega_index = 0;
	for(int32_t mega_y = 0; mega_y < height / 4; mega_y += 64) {
		for(int32_t mega_x = 0; mega_x < width / 4; mega_x += 64) {
			for(int32_t block_y = 0; block_y < 64; block_y++) {
				for(int32_t block_x = 0; block_x < 64; block_x++) {
					int32_t dest_x = mega_x + block_x;
					int32_t dest_y = mega_y + block_y;
					
					int32_t dest_index = block_y * 64 + block_x;
					int32_t src_index = swizzle_table[dest_index] - 1;
					
					int32_t src_x = (src_mega_index + src_index) % (width / 4);
					int32_t src_y = (src_mega_index + src_index) / (width / 4);
					
					copy_block(dest, dest_x, dest_y, src, src_x, src_y, 4, 4, width);
				}
			}
			src_mega_index += 4096;
		}
	}
}

void write_png(const char* filename, const unsigned char* image, unsigned w, unsigned h) {
	lodepng_encode32_file(filename, image, w, h);
}

}
