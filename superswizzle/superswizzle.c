#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "tables.inl"
#include "dxgi_format.inl"

#define verify(cond, msg) if(!(cond)) { printf(msg "\n"); exit(1); }
#define check_fread(num) if((num) != 1) { printf("error: fread failed (line %d)\n", __LINE__); exit(1); }

typedef struct {
	int32_t type_hash;
	int32_t offset;
	int32_t size;
	uint8_t* data;
} Section;

typedef struct {
	int32_t asset_type_hash;
	int32_t section_count;
	Section* sections;
} DatFile;

typedef struct {
	uint32_t unk_0;
	uint32_t unk_4;
	int16_t width;
	int16_t height;
	uint32_t unk_c;
	uint32_t unk_10;
	uint8_t format;
} TextureHeader;

static DatFile parse_dat_file(const char* path);
static uint8_t* load_last_n_bytes(const char* path, int32_t n);
static void crop_image_in_place(uint8_t* image, int32_t new_width, int32_t new_height, int32_t old_width, int32_t old_height);

void decode_init();
void decode_bc1(uint8_t* dest, uint8_t* src, int32_t width, int32_t height);
void decode_bc4(uint8_t* dest, uint8_t* src, int32_t width, int32_t height);
void decode_bc7(uint8_t* dest, uint8_t* src, int32_t width, int32_t height);
void unswizzle(uint8_t* dest, uint8_t* src, int32_t width, int32_t height, const int32_t* swizzle_table);
void write_png(const char* filename, const unsigned char* image, unsigned w, unsigned h);

int main(int argc, char** argv)
{
	if(argc != 2 && argc != 4) {
		printf("superswizzle -- https://github.com/chaoticgd/ripped_apart\n");
		printf("usage: %s <.texture input path>\n", argv[0]);
		printf("       %s <.texture input path> <.stream input path> <.png output path>\n", argv[0]);
		return 1;
	}
	
	// Parse the arguments, determine the paths of the input and output files.
	const char* texture_file = argv[1];
	const char* stream_file;
	const char* output_file;
	
	if(argc == 2) {
		char* stream_file_auto = malloc(strlen(texture_file) + strlen(".stream") + 1);
		strcpy(stream_file_auto, texture_file);
		memcpy(stream_file_auto + strlen(stream_file_auto), ".stream", strlen(".stream") + 1);
		stream_file = stream_file_auto;
		
		char* output_file_auto = malloc(strlen(texture_file) + strlen(".png") + 1);
		strcpy(output_file_auto, texture_file);
		memcpy(output_file_auto + strlen(output_file_auto), ".png", strlen(".png") + 1);
		output_file = output_file_auto;
	} else if(argc == 4) {
		stream_file = argv[2];
		output_file = argv[3];
	}
	
	// Parse the container format.
	DatFile dat = parse_dat_file(texture_file);
	
	printf("asset type %x\n", dat.asset_type_hash);
	
	for(int32_t i = 0; i < dat.section_count; i++) {
		printf("section of type %x @ %x\n", dat.sections[i].type_hash, dat.sections[i].offset);
	}
	
	verify(dat.section_count > 0 && dat.sections[0].type_hash == 0x4ede3593, "Bad sections.");
	TextureHeader* tex_header = (TextureHeader*) dat.sections[0].data;
	
	int32_t real_width = (int32_t) pow(2, ceilf(log2(tex_header->width)));
	int32_t real_height = (int32_t) pow(2, ceilf(log2(tex_header->height)));
	
	verify(real_width % 256 == 0, "error: Texture size not supported.");
	verify(real_height % 256 == 0, "error: Texture size not supported.");
	
	printf("width: %hd\n", tex_header->width);
	printf("height: %hd\n", tex_header->height);
	printf("format: %hhd\n", tex_header->format);
	
	int32_t pixel_count = real_width * real_height;
	
	int32_t texture_size;
	switch(tex_header->format) {
		//case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		//case DXGI_FORMAT_R8G8B8A8_UNORM:
		//case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		//case DXGI_FORMAT_R8G8B8A8_UINT:
		//case DXGI_FORMAT_R8G8B8A8_SNORM:
		//case DXGI_FORMAT_R8G8B8A8_SINT: {
		//	texture_size = pixel_count * 4;
		//	break;
		//}
		//case DXGI_FORMAT_R8G8_TYPELESS:
		//case DXGI_FORMAT_R8G8_UNORM:
		//case DXGI_FORMAT_R8G8_UINT:
		//case DXGI_FORMAT_R8G8_SNORM:
		//case DXGI_FORMAT_R8G8_SINT: {
		//	texture_size = pixel_count * 2;
		//	break;
		//}
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
	uint8_t* compressed = load_last_n_bytes(stream_file, texture_size);
	if(compressed == NULL) {
		compressed = load_last_n_bytes(texture_file, texture_size);
		verify(compressed != NULL, "error: Failed to read texture data.\n");
	}
	
	// Allocate memory for decompression and unswizzling.
	uint8_t* decompressed = malloc(real_width * real_height * 4 + 256 * 245);
	uint8_t* unswizzled = malloc(real_width * real_height * 4);
	
	// Decompress and unswizzle the textures.
	decode_init();
	switch(tex_header->format) {
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT: {
			memcpy(unswizzled, compressed, pixel_count * 4);
			//unswizzle(unswizzled, compressed, real_width, real_height, SWIZZLE_TABLE_2);
			break;
		}
		case DXGI_FORMAT_R8G8_TYPELESS:
		case DXGI_FORMAT_R8G8_UNORM:
		case DXGI_FORMAT_R8G8_UINT:
		case DXGI_FORMAT_R8G8_SNORM:
		case DXGI_FORMAT_R8G8_SINT: {
			for(int32_t i = 0; i < real_width * real_height; i++) {
				unswizzled[i * 4 + 0] = compressed[i * 2 + 0];
				unswizzled[i * 4 + 1] = compressed[i * 2 + 1];
				unswizzled[i * 4 + 2] = 0x00;
				unswizzled[i * 4 + 3] = 0xff;
			}
			//unswizzle(unswizzled, decompressed, real_width, real_height, SWIZZLE_TABLE_1);
			break;
		}
		case DXGI_FORMAT_BC1_TYPELESS:
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB: {
			decode_bc1(decompressed, compressed, real_width, real_height);
			unswizzle(unswizzled, decompressed, real_width, real_height, SWIZZLE_TABLE_1);
			break;
		}
		case DXGI_FORMAT_BC4_TYPELESS:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_SNORM: {
			decode_bc4(decompressed, compressed, real_width, real_height);
			unswizzle(unswizzled, decompressed, real_width, real_height, SWIZZLE_TABLE_1);
			break;
		}
		//case DXGI_FORMAT_BC6H_UF16: {
		//	break;
		//}
		case DXGI_FORMAT_BC7_TYPELESS:
		case DXGI_FORMAT_BC7_UNORM:
		case DXGI_FORMAT_BC7_UNORM_SRGB: {
			decode_bc7(decompressed, compressed, real_width, real_height);
			unswizzle(unswizzled, decompressed, real_width, real_height, SWIZZLE_TABLE_2);
			break;
		}
	}
	
	free(compressed);
	free(decompressed);
	
	if(real_width != tex_header->width || real_height != tex_header->height) {
		assert(real_width >= tex_header->width && real_height >= tex_header->height);
		crop_image_in_place(unswizzled, tex_header->width, tex_header->height, real_width, real_height);
	}
	
	// Write the output file.
	write_png(output_file, unswizzled, tex_header->width, tex_header->height);
	
	free(unswizzled);
	
	return 0;
}

typedef struct {
	int32_t type_hash;
	int32_t offset;
	int32_t size;
} SectionHeader;

typedef struct {
	char magic[4];
	int32_t asset_type_hash;
	int32_t file_size;
	int16_t section_count;
	int16_t shader_count;
} DatHeader;

static DatFile parse_dat_file(const char* path) {
	DatFile dat;
	FILE* file = fopen(path, "rb");
	if(!file) {
		printf("error: Failed to open '%s' for reading.\n", path);
		exit(1);
	}
	DatHeader header;
	check_fread(fread(&header, sizeof(DatHeader), 1, file));
	dat.asset_type_hash = header.asset_type_hash;
	dat.section_count = header.section_count;
	verify(dat.section_count > 0, "error: No sections!");
	verify(dat.section_count < 1000, "error: Too many sections!");
	dat.sections = malloc(sizeof(Section) * header.section_count);
	SectionHeader* headers = malloc(sizeof(SectionHeader) * header.section_count);
	check_fread(fread(headers, dat.section_count * sizeof(SectionHeader), 1, file));
	for(int32_t i = 0; i < header.section_count; i++) {
		dat.sections[i].type_hash = headers[i].type_hash;
		dat.sections[i].offset = headers[i].offset;
		dat.sections[i].size = headers[i].size;
		verify(headers[i].size < 1024 * 1024 * 1024, "error: Section too big!");
		dat.sections[i].data = malloc(headers[i].size);
		fseek(file, headers[i].offset, SEEK_SET);
		check_fread(fread(dat.sections[i].data, headers[i].size, 1, file));
	}
	free(headers);
	fclose(file);
	return dat;
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
