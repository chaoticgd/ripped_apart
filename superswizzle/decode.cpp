#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>
#include <stdio.h>

#define RGBCX_IMPLEMENTATION
#include "../thirdparty/bc7enc/rgbcx.h"
#include "../thirdparty/bc7enc/bc7decomp.h"
#include "../thirdparty/bc7enc/lodepng.h"

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

void write_png(const char* filename, const unsigned char* image, unsigned w, unsigned h) {
	lodepng_encode32_file(filename, image, w, h);
}

}
