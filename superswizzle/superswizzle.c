#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define BCDEC_IMPLEMENTATION
#include "../thirdparty/bcdec/bcdec.h"
#include "../thirdparty/lodepng/lodepng.h"

#include <libra/dat_container.h>
#include <libra/texture.h>

// Main
int main(int argc, char** argv);
static uint8_t* load_last_n_bytes(const char* path, int32_t n);
static void get_texture_properties(int32_t* block_size, const char** swizzle_pattern, int32_t* bits_per_pixel, int8_t* is_hdr, uint8_t format);
static void decode_and_write_png(const char* output_file, uint8_t* src, int32_t width, int32_t height, int32_t real_width, int32_t real_height, int32_t format, int32_t texture_size, int32_t block_size, const char* pattern);
static void decode_and_write_exr(const char* output_file, uint8_t* src, int32_t width, int32_t height, int32_t real_width, int32_t real_height, int32_t format, int32_t texture_size, int32_t block_size, const char* pattern);
static void crop_image_in_place(uint8_t* image, int32_t new_width, int32_t new_height, int32_t old_width, int32_t old_height, int32_t pixel_size);
// Texture decompression
static void decode_bc1(uint8_t* dest, uint8_t* src, int32_t width, int32_t height);
static void decode_bc3(uint8_t* dest, uint8_t* src, int32_t width, int32_t height);
static void decode_bc4(uint8_t* dest, uint8_t* src, int32_t width, int32_t height);
static void decode_bc5(uint8_t* dest, uint8_t* src, int32_t width, int32_t height);
static void decode_bc6(uint8_t* dest, uint8_t* src, int32_t width, int32_t height, int8_t is_signed);
static void decode_bc7(uint8_t* dest, uint8_t* src, int32_t width, int32_t height);
static void set_block(uint8_t* dest, uint8_t* src, int32_t bx, int32_t by, int32_t bwidth, int32_t bheight, int32_t iwidth, int32_t iheight, int32_t ps);
// Unswizzling
static void unswizzle(uint8_t* dest, const uint8_t* src, int32_t iw, int32_t ih, int32_t bs, int32_t ps, const char* pattern);
static void unswizzle_block(uint8_t* dest, const uint8_t* src, int32_t iw, int32_t ih, int32_t bx, int32_t by, int32_t bs, int32_t* si, int32_t ps, const char* pattern);
// HDR image encoding
static void write_exr(const char* output_path, const uint8_t* src, int32_t width, int32_t height, int8_t is_half);
static void write_exr_attribute(FILE* file, const char* name, const char* type, const void* data, int32_t size);
static void do_undocumented_exr_pixel_reorder(uint8_t* dest, const uint8_t* src, int32_t size);
// Testing
static void test_all_possible_swizzle_patterns(const char* output_file, uint8_t* src, int32_t width, int32_t height, int32_t real_width, int32_t real_height, int32_t format, int32_t texture_size);

// *****************************************************************************
// Main
// *****************************************************************************

int main(int argc, char** argv) {
	if(argc != 2 && argc != 3 && argc != 4) {
		fprintf(stderr, "superswizzle -- https://github.com/chaoticgd/ripped_apart\n");
		fprintf(stderr, "usage: %s <.texture input path>\n", argv[0]);
		fprintf(stderr, "       %s <.texture input path> <.png output path>\n", argv[0]);
		fprintf(stderr, "       %s <.texture input path> <.stream input path> <.png output path>\n", argv[0]);
		return 1;
	}
	
	const char* texture_file = argv[1];
	
	// Parse the container format.
	RA_Result result;
	RA_DatFile dat;
	result = RA_dat_read(&dat, texture_file, 0);
	if(result != NULL) {
		fprintf(stderr, "error: Failed to read texture file header (%s).\n", result->message);
		exit(1);
	}
	
	verify(dat.asset_type_crc == RA_ASSET_TYPE_TEXTURE, "Asset is not a texture");
	
	for(int32_t i = 0; i < dat.lump_count; i++) {
		printf("lump of type %x @ %x\n", dat.lumps[i].type_crc, dat.lumps[i].offset);
	}
	
	verify(dat.lump_count > 0 && dat.lumps[0].type_crc == 0x4ede3593, "error: Bad lumps.");
	RA_TextureHeader* tex_header = (RA_TextureHeader*) dat.lumps[0].data;
	
	printf("width: %hd\n", tex_header->width);
	printf("height: %hd\n", tex_header->height);
	printf("format: %s\n", RA_texture_format_to_string(tex_header->format));
	
	int32_t block_size;
	const char* swizzle_pattern;
	int32_t bits_per_pixel;
	int8_t is_hdr;
	get_texture_properties(&block_size, &swizzle_pattern, &bits_per_pixel, &is_hdr, tex_header->format);
	
	int32_t real_width = tex_header->width;
	int32_t real_height = tex_header->height;
	if((tex_header->format == DXGI_FORMAT_BC4_TYPELESS
		|| tex_header->format == DXGI_FORMAT_BC4_UNORM
		|| tex_header->format == DXGI_FORMAT_BC4_SNORM)
		&& (tex_header->width >= 512 && tex_header->height >= 512)) {
		if(real_width % 512 != 0) real_width += 512 - (real_width % 512);
		if(real_height % 512 != 0) real_height += 512 - (real_height % 512);
	} else {
		if(real_width % block_size != 0) real_width += block_size - (real_width % block_size);
		if(real_height % block_size != 0) real_height += block_size - (real_height % block_size);
	}
	int32_t texture_size = (real_width * real_height * bits_per_pixel) / 8;
	
	// Parse the rest of the arguments, determine the paths of the input and
	// output files.
	const char* stream_file;
	const char* output_file;
	
	const char* extension = is_hdr ? ".exr" : ".png";
	
	if(argc == 2 || argc == 3) {
		char* stream_file_auto = malloc(strlen(texture_file) + strlen(".stream") + 1);
		strcpy(stream_file_auto, texture_file);
		memcpy(stream_file_auto + strlen(stream_file_auto), ".stream", strlen(".stream") + 1);
		stream_file = stream_file_auto;
		
		if(argc == 2) {
			char* output_file_auto = malloc(strlen(texture_file) + strlen(extension) + 1);
			strcpy(output_file_auto, texture_file);
			memcpy(output_file_auto + strlen(output_file_auto), extension, strlen(extension) + 1);
			output_file = output_file_auto;
		} else {
			output_file = argv[2];
		}
	} else if(argc == 4) {
		stream_file = argv[2];
		output_file = argv[3];
	}
	
	// Read the highest mip from disk.
	uint8_t* compressed = NULL;
	if(tex_header->width == tex_header->width_in_texture_file
		&& tex_header->height == tex_header->height_in_texture_file) {
		compressed = load_last_n_bytes(texture_file, texture_size);
		if(compressed == NULL) {
			verify(compressed != NULL, "error: Failed to read texture data (from .texture file).");
		}
	} else {
		compressed = load_last_n_bytes(stream_file, texture_size);
		if(compressed == NULL) {
			verify(compressed != NULL, "error: Failed to read texture data. Missing or corrupted .stream file?");
		}
	}
	
	// Decompress the image, unswizzle it and write the output file.
	if(is_hdr) {
		decode_and_write_exr(output_file, compressed, tex_header->width, tex_header->height, real_width, real_height, tex_header->format, texture_size, block_size, swizzle_pattern);
	} else {
		decode_and_write_png(output_file, compressed, tex_header->width, tex_header->height, real_width, real_height, tex_header->format, texture_size, block_size, swizzle_pattern);
	}
	
	//test_all_possible_swizzle_patterns(output_file, compressed, tex_header->width, tex_header->height, real_width, real_height, tex_header->format, texture_size);
	
	free(compressed);
	
	return 0;
}

static void get_texture_properties(int32_t* block_size, const char** swizzle_pattern, int32_t* bits_per_pixel, int8_t* is_hdr, uint8_t format) {
	switch(format) {
		case DXGI_FORMAT_R16G16B16A16_FLOAT: {
			*block_size = 64;
			*swizzle_pattern = "2Z2Z2Z2Z2N2Z";
			*bits_per_pixel = 64;
			*is_hdr = 1;
			break;
		}
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT: {
			*block_size = 128;
			*swizzle_pattern = "2N2N2N2N2N4Z";
			*bits_per_pixel = 32;
			*is_hdr = 0;
			break;
		}
		case DXGI_FORMAT_R8G8_TYPELESS:
		case DXGI_FORMAT_R8G8_UNORM:
		case DXGI_FORMAT_R8G8_UINT:
		case DXGI_FORMAT_R8G8_SNORM:
		case DXGI_FORMAT_R8G8_SINT: {
			*block_size = 128;
			*swizzle_pattern = "2Z2Z2Z2Z8Z";
			*bits_per_pixel = 16;
			*is_hdr = 0;
			break;
		}
		case DXGI_FORMAT_R8_TYPELESS:
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_R8_UINT:
		case DXGI_FORMAT_R8_SNORM:
		case DXGI_FORMAT_R8_SINT: {
			*block_size = 256;
			*swizzle_pattern = "2N2N2N2NXZ";
			*bits_per_pixel = 8;
			*is_hdr = 0;
			break;
		}
		case DXGI_FORMAT_BC1_TYPELESS:
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB: {
			*block_size = 256;
			*swizzle_pattern = "2Z2Z2Z2Z2N2Z";
			*bits_per_pixel = 4;
			*is_hdr = 0;
			break;
		}
		case DXGI_FORMAT_BC3_TYPELESS:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC3_UNORM_SRGB: {
			*block_size = 256;
			*swizzle_pattern = "2N2N2N2N4N";
			*bits_per_pixel = 8;
			*is_hdr = 0;
			break;
		}
		case DXGI_FORMAT_BC4_TYPELESS:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_SNORM: {
			*block_size = 256;
			*swizzle_pattern = "2Z2Z2Z2Z2N2Z";
			*bits_per_pixel = 4;
			*is_hdr = 0;
			break;
		}
		case DXGI_FORMAT_BC5_TYPELESS:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC5_SNORM: {
			*block_size = 256;
			*swizzle_pattern = "2N2N2N2N4N";
			*bits_per_pixel = 8;
			*is_hdr = 0;
			break;
		}
		case DXGI_FORMAT_BC6H_TYPELESS:
		case DXGI_FORMAT_BC6H_UF16:
		case DXGI_FORMAT_BC6H_SF16: {
			*block_size = 256;
			*swizzle_pattern = "2N2N2N2N4N";
			*bits_per_pixel = 8;
			*is_hdr = 1;
			break;
		}
		case DXGI_FORMAT_BC7_TYPELESS:
		case DXGI_FORMAT_BC7_UNORM:
		case DXGI_FORMAT_BC7_UNORM_SRGB: {
			*block_size = 256;
			*swizzle_pattern = "2N2N2N2N4N";
			*bits_per_pixel = 8;
			*is_hdr = 0;
			break;
		}
		default: {
			printf("error: Unsupported texture format!\n");
			exit(1);
		}
	}
}

static uint8_t* load_last_n_bytes(const char* path, int32_t n) {
	FILE* file = fopen(path, "rb");
	if(!file) {
		return NULL;
	}
	fseek(file, -n, SEEK_END);
	printf("mip @ %lx\n", ftell(file));
	uint8_t* buffer = malloc(n);
	check_fread(fread(buffer, n, 1, file));
	fclose(file);
	return buffer;
}

static void decode_and_write_png(const char* output_file, uint8_t* src, int32_t width, int32_t height, int32_t real_width, int32_t real_height, int32_t format, int32_t texture_size, int32_t block_size, const char* pattern) {
	// Allocate memory for decompression and unswizzling.
	uint8_t* decompressed = malloc(real_width * real_height * 4);
	uint8_t* unswizzled = malloc(real_width * real_height * 4);
	
	// Decompress and unswizzle the textures.
	switch(format) {
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT: {
			unswizzle(unswizzled, src, real_width, real_height, block_size, 4, pattern);
			break;
		}
		case DXGI_FORMAT_R8G8_TYPELESS:
		case DXGI_FORMAT_R8G8_UNORM:
		case DXGI_FORMAT_R8G8_UINT:
		case DXGI_FORMAT_R8G8_SNORM:
		case DXGI_FORMAT_R8G8_SINT: {
			for(int32_t i = 0; i < real_width * real_height; i++) {
				decompressed[i * 4 + 0] = src[i * 2 + 0];
				decompressed[i * 4 + 1] = src[i * 2 + 1];
				decompressed[i * 4 + 2] = 0x00;
				decompressed[i * 4 + 3] = 0xff;
			}
			unswizzle(unswizzled, decompressed, real_width, real_height, block_size, 4, pattern);
			break;
		}
		case DXGI_FORMAT_R8_TYPELESS:
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_R8_UINT:
		case DXGI_FORMAT_R8_SNORM:
		case DXGI_FORMAT_R8_SINT: {
			for(int32_t i = 0; i < real_width * real_height; i++) {
				decompressed[i * 4 + 0] = src[i];
				decompressed[i * 4 + 1] = src[i];
				decompressed[i * 4 + 2] = src[i];
				decompressed[i * 4 + 3] = 0xff;
			}
			unswizzle(unswizzled, decompressed, real_width, real_height, block_size, 4, pattern);
			break;
		}
		case DXGI_FORMAT_BC1_TYPELESS:
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB: {
			decode_bc1(decompressed, src, real_width, real_height);
			unswizzle(unswizzled, decompressed, real_width, real_height, block_size, 4, pattern);
			break;
		}
		case DXGI_FORMAT_BC3_TYPELESS:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC3_UNORM_SRGB: {
			decode_bc3(decompressed, src, real_width, real_height);
			unswizzle(unswizzled, decompressed, real_width, real_height, block_size, 4, pattern);
			break;
		}
		case DXGI_FORMAT_BC4_TYPELESS:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_SNORM: {
			decode_bc4(decompressed, src, real_width, real_height);
			unswizzle(unswizzled, decompressed, real_width, real_height, block_size, 4, pattern);
			break;
		}
		case DXGI_FORMAT_BC5_TYPELESS:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC5_SNORM: {
			decode_bc5(decompressed, src, real_width, real_height);
			unswizzle(unswizzled, decompressed, real_width, real_height, block_size, 4, pattern);
			break;
		}
		case DXGI_FORMAT_BC7_TYPELESS:
		case DXGI_FORMAT_BC7_UNORM:
		case DXGI_FORMAT_BC7_UNORM_SRGB: {
			decode_bc7(decompressed, src, real_width, real_height);
			unswizzle(unswizzled, decompressed, real_width, real_height, block_size, 4, pattern);
			break;
		}
	}
	
	free(decompressed);
	
	if(real_width != width || real_height != height) {
		assert(real_width >= width && real_height >= height);
		crop_image_in_place(unswizzled, width, height, real_width, real_height, 4);
	}
	
	// Write the output file.
	lodepng_encode32_file(output_file, unswizzled, width, height);
	
	free(unswizzled);
}

static void decode_and_write_exr(const char* output_file, uint8_t* src, int32_t width, int32_t height, int32_t real_width, int32_t real_height, int32_t format, int32_t texture_size, int32_t block_size, const char* pattern) {
	// Allocate memory for decompression and unswizzling.
	uint8_t* decompressed = malloc(real_width * real_height * 16);
	uint8_t* unswizzled = malloc(real_width * real_height * 16);
	int8_t is_half = 0;
	
	// Decompress and unswizzle the textures.
	switch(format) {
		case DXGI_FORMAT_R16G16B16A16_FLOAT: {
			unswizzle(unswizzled, src, real_width, real_height, block_size, 8, pattern);
			is_half = 1;
			break;
		}
		case DXGI_FORMAT_BC6H_TYPELESS:
		case DXGI_FORMAT_BC6H_UF16: {
			decode_bc6(decompressed, src, real_width, real_height, 0);
			unswizzle(unswizzled, decompressed, real_width, real_height, block_size, 8, pattern);
			is_half = 1;
			break;
		}
		case DXGI_FORMAT_BC6H_SF16: {
			decode_bc6(decompressed, src, real_width, real_height, 1);
			unswizzle(unswizzled, decompressed, real_width, real_height, block_size, 8, pattern);
			is_half = 1;
			break;
		}
	}
	
	free(decompressed);
	
	if(real_width != width || real_height != height) {
		assert(real_width >= width && real_height >= height);
		crop_image_in_place(unswizzled, width, height, real_width, real_height, is_half ? 8 : 16);
	}
	
	// Write the output file.
	write_exr(output_file, unswizzled, width, height, is_half);
	
	free(unswizzled);
}

static void crop_image_in_place(uint8_t* image, int32_t new_width, int32_t new_height, int32_t old_width, int32_t old_height, int32_t pixel_size) {
	for(int32_t y = 0; y < new_height; y++) {
		for(int32_t x = 0; x < new_width; x++) {
			image[(y * new_width + x) * pixel_size + 0] = image[(y * old_width + x) * pixel_size + 0];
			image[(y * new_width + x) * pixel_size + 1] = image[(y * old_width + x) * pixel_size + 1];
			image[(y * new_width + x) * pixel_size + 2] = image[(y * old_width + x) * pixel_size + 2];
			image[(y * new_width + x) * pixel_size + 3] = image[(y * old_width + x) * pixel_size + 3];
		}
	}
}

// *****************************************************************************
// Texture decompression
// *****************************************************************************

static void decode_bc1(uint8_t* dest, uint8_t* src, int32_t width, int32_t height) {
	for(int32_t y = 0; y < height / 4; y++) {
		for(int32_t x = 0; x < width / 4; x++) {
			uint8_t block[64];
			bcdec_bc1(&src[(y * (width / 4) + x) * 8], block, 16);
			set_block(dest, block, x, y, 4, 4, width, height, 4);
		}
	}
}

static void decode_bc3(uint8_t* dest, uint8_t* src, int32_t width, int32_t height) {
	for(int32_t y = 0; y < height / 4; y++) {
		for(int32_t x = 0; x < width / 4; x++) {
			uint8_t block[64];
			bcdec_bc3(&src[(y * (width / 4) + x) * 16], block, 16);
			set_block(dest, block, x, y, 4, 4, width, height, 4);
		}
	}
}

static void decode_bc4(uint8_t* dest, uint8_t* src, int32_t width, int32_t height) {
	for(int32_t y = 0; y < height / 4; y++) {
		for(int32_t x = 0; x < width / 4; x++) {
			uint8_t r_block[16];
			bcdec_bc4(&src[(y * (width / 4) + x) * 8], r_block, 4);
			uint8_t block[64];
			// R???? -> RRR1
			for(int32_t i = 0; i < 16; i++) {
				block[i * 4 + 0] = r_block[i];
				block[i * 4 + 1] = r_block[i];
				block[i * 4 + 2] = r_block[i];
				block[i * 4 + 3] = 0xff;
			}
			set_block(dest, block, x, y, 4, 4, width, height, 4);
		}
	}
}

static void decode_bc5(uint8_t* dest, uint8_t* src, int32_t width, int32_t height) {
	for(int32_t y = 0; y < height / 4; y++) {
		for(int32_t x = 0; x < width / 4; x++) {
			uint8_t rg_block[32];
			bcdec_bc5(&src[(y * (width / 4) + x) * 16], rg_block, 8);
			uint8_t block[64];
			for(int32_t i = 0; i < 16; i++) {
				block[i * 4 + 0] = rg_block[i * 2 + 0];
				block[i * 4 + 1] = rg_block[i * 2 + 0];
				block[i * 4 + 2] = 0x00;
				block[i * 4 + 3] = 0xff;
			}
			set_block(dest, block, x, y, 4, 4, width, height, 4);
		}
	}
}

static void decode_bc6(uint8_t* dest, uint8_t* src, int32_t width, int32_t height, int8_t is_signed) {
	for(int32_t y = 0; y < height / 4; y++) {
		for(int32_t x = 0; x < width / 4; x++) {
			uint8_t rgb_block[96];
			bcdec_bc6h_half(&src[(y * (width / 4) + x) * 16], rgb_block, 12, is_signed);
			uint8_t block[128];
			// RGBRGB -> RGB1RGB1
			for(int32_t i = 0; i < 16; i++) {
				block[i * 8 + 0] = rgb_block[i * 6 + 0];
				block[i * 8 + 1] = rgb_block[i * 6 + 1];
				block[i * 8 + 2] = rgb_block[i * 6 + 2];
				block[i * 8 + 3] = rgb_block[i * 6 + 3];
				block[i * 8 + 4] = rgb_block[i * 6 + 4];
				block[i * 8 + 5] = rgb_block[i * 6 + 5];
				block[i * 8 + 6] = 0x00; // (half) 1.f
				block[i * 8 + 7] = 0x3c;
			}
			set_block(dest, block, x, y, 4, 4, width, height, 8);
		}
	}
}

static void decode_bc7(uint8_t* dest, uint8_t* src, int32_t width, int32_t height) {
	for(int32_t y = 0; y < height / 4; y++) {
		for(int32_t x = 0; x < width / 4; x++) {
			uint8_t block[64];
			bcdec_bc7(&src[(y * (width / 4) + x) * 16], block, 16);
			set_block(dest, block, x, y, 4, 4, width, height, 4);
		}
	}
}

static void set_block(uint8_t* dest, uint8_t* src, int32_t bx, int32_t by, int32_t bwidth, int32_t bheight, int32_t iwidth, int32_t iheight, int32_t ps) {
	int32_t origin_x = bx * bwidth;
	int32_t origin_y = by * bheight;
	for(int32_t y = 0; y < bheight; y++) {
		memcpy(&dest[((origin_y + y) * iwidth + origin_x) * ps], &src[(y * bwidth) * ps], bwidth * ps);
	}
}

// *****************************************************************************
// Unswizzling
// *****************************************************************************

static void unswizzle(uint8_t* dest, const uint8_t* src, int32_t iw, int32_t ih, int32_t bs, int32_t ps, const char* pattern) {
	int32_t si = 0;
	for(int32_t by = 0; by < ih; by += bs) {
		for(int32_t bx = 0; bx < iw; bx += bs) {
			unswizzle_block(dest, src, iw, ih, bx, by, bs, &si, ps, pattern);
		}
	}
}

static void unswizzle_block(uint8_t* dest, const uint8_t* src, int32_t iw, int32_t ih, int32_t bx, int32_t by, int32_t bs, int32_t* si, int32_t ps, const char* pattern) {
	if(pattern[0] == '\0') {
		// Base case: copy a block.
		int32_t sx = (*si % (iw / bs)) * bs;
		int32_t sy = (*si / (iw / bs)) * bs;
		for(int32_t y = 0; y < bs; y++) {
			memcpy(
				&dest[((by + y) * iw + bx) * ps],
				&src[((sy + y) * iw + sx) * ps],
				bs * ps);
		}
		*si += 1;
		return;
	} else {
		// Recursive case: subdivide factor*factor times.
		int32_t factor = pattern[0] - '0';
		if(pattern[0] == 'X') factor = 16;
		if(pattern[1] == 'N') {
			for(int32_t x = 0; x < factor; x++) {
				for(int32_t y = 0; y < factor; y++) {
					unswizzle_block(dest, src, iw, ih, bx + bs * (x / (float)factor), by + bs * (y / (float)factor), bs / factor, si, ps, pattern + 2);
				}
			}
			return;
		} else if(pattern[1] == 'Z') {
			for(int32_t y = 0; y < factor; y++) {
				for(int32_t x = 0; x < factor; x++) {
					unswizzle_block(dest, src, iw, ih, bx + bs * (x / (float)factor), by + bs * (y / (float)factor), bs / factor, si, ps, pattern + 2);
				}
			}
			return;
		}
	}
	fprintf(stderr, "error: Bad swizzle format string specified! This should never happen!");
	exit(1);
}

// *****************************************************************************
// HDR image encoding
// *****************************************************************************

typedef struct {
	char name[2];
	char type[4];
#define EXR_CHANNEL_TYPE_UINT 0
#define EXR_CHANNEL_TYPE_HALF 1
#define EXR_CHANNEL_TYPE_FLOAT 2
	char linear;
	char reserved[3];
	char x_sampling[4];
	char y_sampling[4];
} ExrChannel;

typedef struct {
	int32_t x_min;
	int32_t y_min;
	int32_t x_max;
	int32_t y_max;
} ExrBox2i;

typedef struct {
	float x;
	float y;
} ExrV2f;

static void write_exr(const char* output_path, const uint8_t* src, int32_t width, int32_t height, int8_t is_half) {
	FILE* file = fopen(output_path, "wb");
	verify(file, "Failed to write output file.");
	
	// magic number
	check_fputc(fputc('\x76', file));
	check_fputc(fputc('\x2f', file));
	check_fputc(fputc('\x31', file));
	check_fputc(fputc('\x01', file));
	
	// version
	check_fputc(fputc('\x02', file));
	check_fputc(fputc('\x00', file));
	check_fputc(fputc('\x00', file));
	check_fputc(fputc('\x00', file));
	
	// header
	ExrChannel channels[5];
	memset(channels, 0, sizeof(channels));
	static const char CHANNEL_NAMES[4] = {'A', 'B', 'G', 'R'};
	for(int32_t i = 0; i < 4; i++) {
		channels[i].name[0] = CHANNEL_NAMES[i];
		channels[i].type[0] = is_half ? EXR_CHANNEL_TYPE_HALF : EXR_CHANNEL_TYPE_FLOAT;
		channels[i].linear = 1;
		channels[i].x_sampling[0] = 1;
		channels[i].y_sampling[0] = 1;
	}
	write_exr_attribute(file, "channels", "chlist", channels, 4 * sizeof(ExrChannel) + 1);
	
	int8_t deflate = 1;
	char compression = deflate ? 3 : 0;
	write_exr_attribute(file, "compression", "compression", &compression, sizeof(compression));
	
	int32_t data_window[4] = {0, 0, width - 1, height - 1};
	write_exr_attribute(file, "dataWindow", "box2i", data_window, sizeof(data_window));
	
	int32_t display_window[4] = {0, 0, width - 1, height - 1};
	write_exr_attribute(file, "displayWindow", "box2i", display_window, sizeof(display_window));
	
	char line_order = 0;
	write_exr_attribute(file, "lineOrder", "lineOrder", &line_order, sizeof(line_order));
	
	float pixel_aspect_ratio = 1.f;
	write_exr_attribute(file, "pixelAspectRatio", "float", &pixel_aspect_ratio, sizeof(pixel_aspect_ratio));
	
	ExrV2f screen_window_center = {0.f, 0.f};
	write_exr_attribute(file, "screenWindowCenter", "v2f", &screen_window_center, sizeof(screen_window_center));
	
	float screen_window_width = 1.f;
	write_exr_attribute(file, "screenWindowWidth", "float", &screen_window_width, sizeof(screen_window_width));
	
	// end of header
	check_fputc(fputc('\x00', file));
	
	// offset table
	int32_t max_rows_per_chunk = deflate ? 16 : 1;
	int32_t chunk_count = (height + max_rows_per_chunk - 1) / max_rows_per_chunk;
	int32_t offset_table_size = chunk_count * 8;
	int64_t* offset_table = malloc(offset_table_size);
	int64_t offset_table_pos = ftell(file);
	check_fseek(fseek(file, offset_table_size, SEEK_CUR));
	
	// pixel data
	int32_t bytes_per_channel = is_half ? 2 : 4;
	int32_t bytes_per_pixel = is_half ? 8 : 16;
	int32_t row_size = width * bytes_per_pixel;
	int32_t chunk_data_size = row_size * max_rows_per_chunk;
	uint8_t* chunk_data = malloc(row_size * max_rows_per_chunk);
	uint8_t* reordered_chunk_data = malloc(row_size * max_rows_per_chunk);
	LodePNGCompressSettings compression_settings;
	lodepng_compress_settings_init(&compression_settings);
	for(int32_t chunk = 0; chunk < chunk_count; chunk++) {
		int32_t base_y = chunk * max_rows_per_chunk;
		
		// RGBARGBARGBARGBA -> AABBGGRRAABBGGRR
		int32_t chunk_height = height - base_y;
		if(chunk_height > max_rows_per_chunk) chunk_height = max_rows_per_chunk;
		for(int32_t y = 0; y < chunk_height; y++) {
			for(int32_t x = 0; x < width; x++) {
				for(int32_t channel = 0; channel < 4; channel++) {
					memcpy(
						&chunk_data[y * row_size + (channel * width + x) * bytes_per_channel],
						&src[((base_y + y) * width + x) * bytes_per_pixel + (3 - channel) * bytes_per_channel],
						bytes_per_channel);
				}
			}
		}
		
		offset_table[chunk] = ftell(file);
		
		check_fwrite(fwrite(&base_y, sizeof(base_y), 1, file));
		if(deflate) {
			do_undocumented_exr_pixel_reorder(reordered_chunk_data, chunk_data, chunk_data_size);
			
			uint8_t* compressed_chunk_data;
			size_t compressed_chunk_data_size;
			lodepng_zlib_compress(&compressed_chunk_data, &compressed_chunk_data_size, reordered_chunk_data, chunk_data_size, &compression_settings);
			
			check_fwrite(fwrite(&compressed_chunk_data_size, 4, 1, file));
			check_fwrite(fwrite(compressed_chunk_data, compressed_chunk_data_size, 1, file));
			
			free(compressed_chunk_data);
		} else {
			check_fwrite(fwrite(&chunk_data_size, sizeof(chunk_data_size), 1, file));
			check_fwrite(fwrite(chunk_data, chunk_data_size, 1, file));
		}
	}
	
	free(chunk_data);
	free(reordered_chunk_data);
	
	// write offset table
	check_fseek(fseek(file, offset_table_pos, SEEK_SET));
	check_fwrite(fwrite(offset_table, offset_table_size, 1, file));
	free(offset_table);
	
	fclose(file);
}

static void write_exr_attribute(FILE* file, const char* name, const char* type, const void* data, int32_t size) {
	check_fputs(fputs(name, file));
	check_fputc(fputc('\x00', file));
	check_fputs(fputs(type, file));
	check_fputc(fputc('\x00', file));
	check_fwrite(fwrite(&size, 4, 1, file));
	check_fwrite(fwrite(data, size, 1, file));
}

static void do_undocumented_exr_pixel_reorder(uint8_t* dest, const uint8_t* src, int32_t size) {
	if(size == 0) {
		return;
	}
	
	// Reorder the pixel data.
	uint8_t* t1 = dest;
	uint8_t* t2 = dest + (size + 1) / 2;
	const uint8_t* src_end = src + size;
	while(1) {
		if(src < src_end)
			*(t1++) = *(src++);
		else
			break;
		if(src < src_end)
			*(t2++) = *(src++);
		else
			break;
	}
	
	// Predictor.
	uint8_t* t = (uint8_t*) dest + 1;
	uint8_t* dest_end = (uint8_t*) dest + size;
	uint8_t p = t[-1];
	while(t < dest_end) {
		int d = t[0] - p + (128 + 256);
		p = t[0];
		t[0] = d;
		++t;
	}
}

// *****************************************************************************
// Testing
// *****************************************************************************

// This is how I originally found the swizzle patterns. It's kept here in case
// it ends up being useful in the future.
#if 0
static void test_all_possible_swizzle_patterns(const char* output_file, uint8_t* src, int32_t width, int32_t height, int32_t real_width, int32_t real_height, int32_t format, int32_t texture_size) {
	static char pattern[128] = {0};
	static int32_t bs = 64;
	static int32_t ofs = 0;
	
	if(bs < 1) {
		return;
	} else if(bs == 1) {
		// R16G16B16A16
		if(pattern[0] != '2')return;
		if(pattern[1] != 'Z')return;
		if(pattern[2] != '2')return;
		if(pattern[3] != 'Z')return;
		
		pattern[ofs] = '\0';
		char* testout = malloc(strlen(output_file)+sizeof(pattern)+4+1);
		memcpy(testout, output_file, strlen(output_file));
		memcpy(testout+strlen(output_file), pattern, strlen(pattern));
		memcpy(testout+strlen(output_file)+strlen(pattern), ".exr", 5);
		decode_and_write_exr(testout, src, width, height, real_width, real_height, format, texture_size, 64, pattern);
	} else {
		// Try subdiv 2.
		int32_t before_bs = bs;
		bs /= 2;
		ofs += 2;
		
		pattern[ofs - 2] = '2';
		
		pattern[ofs - 1] = 'Z';
		test_all_possible_swizzle_patterns(output_file, src, width, height, real_width, real_height, format, texture_size);
		pattern[ofs - 1] = 'N';
		test_all_possible_swizzle_patterns(output_file, src, width, height, real_width, real_height, format, texture_size);
		
		// Try subdiv 4.
		bs /= 2;
		
		pattern[ofs - 2] = '4';
		
		pattern[ofs - 1] = 'Z';
		test_all_possible_swizzle_patterns(output_file, src, width, height, real_width, real_height, format, texture_size);
		pattern[ofs - 1] = 'N';
		test_all_possible_swizzle_patterns(output_file, src, width, height, real_width, real_height, format, texture_size);
		
		// Try subdiv 8.
		bs /= 2;
		
		pattern[ofs - 2] = '8';
		
		pattern[ofs - 1] = 'Z';
		test_all_possible_swizzle_patterns(output_file, src, width, height, real_width, real_height, format, texture_size);
		pattern[ofs - 1] = 'N';
		test_all_possible_swizzle_patterns(output_file, src, width, height, real_width, real_height, format, texture_size);
		
		// Try subdiv 16.
		bs /= 2;
		
		pattern[ofs - 2] = 'X';
		
		pattern[ofs - 1] = 'Z';
		test_all_possible_swizzle_patterns(output_file, src, width, height, real_width, real_height, format, texture_size);
		pattern[ofs - 1] = 'N';
		test_all_possible_swizzle_patterns(output_file, src, width, height, real_width, real_height, format, texture_size);
		
		ofs -= 2;
		bs = before_bs;
	}
	
	return;
}
#endif
