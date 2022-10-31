#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <libra/dat_container.h>
#include <libra/texture.h>

static uint8_t* load_last_n_bytes(const char* path, int32_t n);
static void test_all_possible_swizzle_patterns(const char* output_file, uint8_t* src, int32_t width, int32_t height, int32_t real_width, int32_t real_height, int32_t format, int32_t texture_size);
static void decode_and_write_png(const char* output_file, uint8_t* src, int32_t width, int32_t height, int32_t real_width, int32_t real_height, int32_t format, int32_t texture_size, const char* test_pattern);
static void unswizzle(uint8_t* dest, const uint8_t* src, int32_t iw, int32_t ih, int32_t bs, const char* pattern_str);
static void unswizzle_block(uint8_t* dest, const uint8_t* src, int32_t iw, int32_t ih, int32_t bx, int32_t by, int32_t bs, int32_t* si, const char* pattern);
static void crop_image_in_place(uint8_t* image, int32_t new_width, int32_t new_height, int32_t old_width, int32_t old_height);

void decode_init();
void decode_bc1(uint8_t* dest, uint8_t* src, int32_t width, int32_t height);
void decode_bc4(uint8_t* dest, uint8_t* src, int32_t width, int32_t height);
void decode_bc7(uint8_t* dest, uint8_t* src, int32_t width, int32_t height);
void write_png(const char* filename, const unsigned char* image, unsigned w, unsigned h);

int main(int argc, char** argv) {
	if(argc != 2 && argc != 3 && argc != 4) {
		fprintf(stderr, "superswizzle -- https://github.com/chaoticgd/ripped_apart\n");
		fprintf(stderr, "usage: %s <.texture input path>\n", argv[0]);
		fprintf(stderr, "       %s <.texture input path> <.png output path>\n", argv[0]);
		fprintf(stderr, "       %s <.texture input path> <.stream input path> <.png output path>\n", argv[0]);
		return 1;
	}
	
	// Parse the arguments, determine the paths of the input and output files.
	const char* texture_file = argv[1];
	const char* stream_file;
	const char* output_file;
	
	if(argc == 2 || argc == 3) {
		char* stream_file_auto = malloc(strlen(texture_file) + strlen(".stream") + 1);
		strcpy(stream_file_auto, texture_file);
		memcpy(stream_file_auto + strlen(stream_file_auto), ".stream", strlen(".stream") + 1);
		stream_file = stream_file_auto;
		
		if(argc == 2) {
			char* output_file_auto = malloc(strlen(texture_file) + strlen(".png") + 1);
			strcpy(output_file_auto, texture_file);
			memcpy(output_file_auto + strlen(output_file_auto), ".png", strlen(".png") + 1);
			output_file = output_file_auto;
		} else {
			output_file = argv[2];
		}
	} else if(argc == 4) {
		stream_file = argv[2];
		output_file = argv[3];
	}
	
	// Parse the container format.
	RA_Result result;
	RA_DatFile dat;
	result = RA_parse_dat_file(&dat, texture_file);
	if(result != NULL) {
		fprintf(stderr, "error: %s\n", result);
		exit(1);
	}
	
	verify(dat.asset_type_hash == RA_ASSET_TYPE_TEXTURE, "Asset is not a texture");
	
	for(int32_t i = 0; i < dat.lump_count; i++) {
		printf("lump of type %x @ %x\n", dat.lumps[i].type_hash, dat.lumps[i].offset);
	}
	
	verify(dat.lump_count > 0 && dat.lumps[0].type_hash == 0x4ede3593, "error: Bad lumps.");
	RA_TextureHeader* tex_header = (RA_TextureHeader*) dat.lumps[0].data;
	
	int32_t real_width = (int32_t) pow(2, ceilf(log2(tex_header->width)));
	int32_t real_height = (int32_t) pow(2, ceilf(log2(tex_header->height)));
	
	printf("width: %hd\n", tex_header->width);
	printf("height: %hd\n", tex_header->height);
	printf("format: %s\n", RA_texture_format_to_string(tex_header->format));
	
	int32_t pixel_count = real_width * real_height;
	
	int32_t texture_size;
	switch(tex_header->format) {
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT: {
			texture_size = pixel_count * 4;
			break;
		}
		case DXGI_FORMAT_R8G8_TYPELESS:
		case DXGI_FORMAT_R8G8_UNORM:
		case DXGI_FORMAT_R8G8_UINT:
		case DXGI_FORMAT_R8G8_SNORM:
		case DXGI_FORMAT_R8G8_SINT: {
			texture_size = pixel_count * 2;
			break;
		}
		case DXGI_FORMAT_R8_TYPELESS:
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_R8_UINT:
		case DXGI_FORMAT_R8_SNORM:
		case DXGI_FORMAT_R8_SINT: {
			texture_size = pixel_count;
			break;
		}
		case DXGI_FORMAT_BC1_TYPELESS:
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB: {
			texture_size = pixel_count / 2;
			break;
		}
		case DXGI_FORMAT_BC4_TYPELESS:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_SNORM: {
			texture_size = pixel_count / 2;
			break;
		}
		case DXGI_FORMAT_BC7_TYPELESS:
		case DXGI_FORMAT_BC7_UNORM:
		case DXGI_FORMAT_BC7_UNORM_SRGB: {
			texture_size = pixel_count;
			break;
		}
		default: {
			printf("error: Unsupported texture format!\n");
			exit(1);
		}
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
	
	decode_and_write_png(output_file, compressed, tex_header->width, tex_header->height, real_width, real_height, tex_header->format, texture_size, NULL);
	
	//test_all_possible_swizzle_patterns(output_file, compressed, tex_header->width, tex_header->height, real_width, real_height, tex_header->format, texture_size);
	
	free(compressed);
	
	return 0;
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

// This is how I originally found the swizzle patterns. It's kept here in case
// it ends up being useful in the future.
#if 0
static void test_all_possible_swizzle_patterns(const char* output_file, uint8_t* src, int32_t width, int32_t height, int32_t real_width, int32_t real_height, int32_t format, int32_t texture_size) {
	static char pattern[128] = {0};
	static int32_t bs = 256;
	static int32_t ofs = 0;
	
	if(bs < 1) {
		return;
	} else if(bs == 1) {
		// R8
		if(pattern[0] != '2')return;
		if(pattern[0+1] != 'N')return;
		if(pattern[0+2] != '2')return;
		if(pattern[0+3] != 'N')return;
		if(pattern[0+4] != '2')return;
		if(pattern[0+5] != 'N')return;
		
		pattern[ofs] = '\0';
		char* testout = malloc(strlen(output_file)+sizeof(pattern)+4+1);
		memcpy(testout, output_file, strlen(output_file));
		memcpy(testout+strlen(output_file), pattern, strlen(pattern));
		memcpy(testout+strlen(output_file)+strlen(pattern), ".png", 5);
		decode_and_write_png(testout, src, width, height, real_width, real_height, format, texture_size, pattern);
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

static void decode_and_write_png(const char* output_file, uint8_t* src, int32_t width, int32_t height, int32_t real_width, int32_t real_height, int32_t format, int32_t texture_size, const char* test_pattern) {
	// Allocate memory for decompression and unswizzling.
	uint8_t* decompressed = malloc(real_width * real_height * 4);
	uint8_t* unswizzled = malloc(real_width * real_height * 4);
	
	int32_t pixel_count = real_width * real_height;
	
	// Decompress and unswizzle the textures.
	decode_init();
	switch(format) {
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT: {
			//verify(real_width >= 2048 && real_width >= 2048, "Texture too small, not sure how to unswizzle for now.");
			//memcpy(unswizzled, src, real_width*real_height*4);
			unswizzle(unswizzled, src, real_width, real_height, 2048, "XZ2N2N2N2N2N4Z");
			break;
		}
		case DXGI_FORMAT_R8G8_TYPELESS:
		case DXGI_FORMAT_R8G8_UNORM:
		case DXGI_FORMAT_R8G8_UINT:
		case DXGI_FORMAT_R8G8_SNORM:
		case DXGI_FORMAT_R8G8_SINT: {
			for(int32_t i = 0; i < real_width * real_height; i++) {
				unswizzled[i * 4 + 0] = src[i * 2 + 0];
				unswizzled[i * 4 + 1] = src[i * 2 + 1];
				unswizzled[i * 4 + 2] = 0x00;
				unswizzled[i * 4 + 3] = 0xff;
			}
			unswizzle(unswizzled, decompressed, real_width, real_height, 256, "2Z2Z2Z2Z2N2Z");
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
			unswizzle(unswizzled, decompressed, real_width, real_height, 256, "2N2N2N2NXZ");
			break;
		}
		case DXGI_FORMAT_BC1_TYPELESS:
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB: {
			decode_bc1(decompressed, src, real_width, real_height);
			unswizzle(unswizzled, decompressed, real_width, real_height, 256, "2Z2Z2Z2Z2N2Z");
			break;
		}
		case DXGI_FORMAT_BC4_TYPELESS:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_SNORM: {
			decode_bc4(decompressed, src, real_width, real_height);
			unswizzle(unswizzled, decompressed, real_width, real_height, 256, "2Z2Z2Z2Z2N2Z");
			break;
		}
		//case DXGI_FORMAT_BC6H_UF16: {
		//	break;
		//}
		case DXGI_FORMAT_BC7_TYPELESS:
		case DXGI_FORMAT_BC7_UNORM:
		case DXGI_FORMAT_BC7_UNORM_SRGB: {
			decode_bc7(decompressed, src, real_width, real_height);
			unswizzle(unswizzled, decompressed, real_width, real_height, 256, "2N2N2N2N4N");
			break;
		}
	}
	
	free(decompressed);
	
	if(real_width != width || real_height != height) {
		assert(real_width >= width && real_height >= height);
		crop_image_in_place(unswizzled, width, height, real_width, real_height);
	}
	
	// Write the output file.
	write_png(output_file, unswizzled, width, height);
	free(unswizzled);
}

static void unswizzle(uint8_t* dest, const uint8_t* src, int32_t iw, int32_t ih, int32_t bs, const char* pattern_str) {
	char pattern_bytes[128];
	strcpy(pattern_bytes, pattern_str);
	char* pattern = &pattern_bytes[0];
	
	// If the texture is smaller than the block size, we must peel the swizzle
	// pattern until we get one that's small enough.
	while(iw < bs || ih < bs) {
		verify(pattern[0] != '\0', "Texture too small.");
		if(pattern[0] == 'X') {
			pattern[0] = '8';
			bs /= 2;
		} else if(pattern[0] == '8') {
			pattern[0] = '4';
			bs /= 2;
		} else if(pattern[0] == '4') {
			pattern[0] = '2';
			bs /= 2;
		} else {
			pattern = pattern + 2;
			bs /= 2;
		}
	}
	
	// Do the unswizzling.
	int32_t si = 0;
	for(int32_t by = 0; by < ih; by += bs) {
		for(int32_t bx = 0; bx < iw; bx += bs) {
			unswizzle_block(dest, src, iw, ih, bx, by, bs, &si, pattern);
		}
	}
}

static void unswizzle_block(uint8_t* dest, const uint8_t* src, int32_t iw, int32_t ih, int32_t bx, int32_t by, int32_t bs, int32_t* si, const char* pattern) {
	if(pattern[0] == '\0') {
		// Base case: copy a block.
		int32_t sx = (*si % (iw / bs)) * bs;
		int32_t sy = (*si / (iw / bs)) * bs;
		for(int32_t y = 0; y < bs; y++) {
			memcpy(
				&dest[((by + y) * iw + bx) * 4],
				&src[((sy + y) * iw + sx) * 4],
				bs * 4);
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
					unswizzle_block(dest, src, iw, ih, bx + bs * (x / (float)factor), by + bs * (y / (float)factor), bs / factor, si, pattern + 2);
				}
			}
			return;
		} else if(pattern[1] == 'Z') {
			for(int32_t y = 0; y < factor; y++) {
				for(int32_t x = 0; x < factor; x++) {
					unswizzle_block(dest, src, iw, ih, bx + bs * (x / (float)factor), by + bs * (y / (float)factor), bs / factor, si, pattern + 2);
				}
			}
			return;
		}
	}
	fprintf(stderr, "error: Bad swizzle format string specified! This should never happen!");
	exit(1);
}

static void crop_image_in_place(uint8_t* image, int32_t new_width, int32_t new_height, int32_t old_width, int32_t old_height) {
	for(int32_t y = 0; y < new_height; y++) {
		for(int32_t x = 0; x < new_width; x++) {
			image[(y * new_width + x) * 4 + 0] = image[(y * old_width + x) * 4 + 0];
			image[(y * new_width + x) * 4 + 1] = image[(y * old_width + x) * 4 + 1];
			image[(y * new_width + x) * 4 + 2] = image[(y * old_width + x) * 4 + 2];
			image[(y * new_width + x) * 4 + 3] = image[(y * old_width + x) * 4 + 3];
		}
	}
}
