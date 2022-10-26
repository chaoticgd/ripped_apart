#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "tables.inl"

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
	uint8_t type;
} TextureHeader;

#define TEX_BC1 0x48
#define TEX_BC7 0x63

static DatFile parse_dat_file(const char* path);
static uint8_t* load_last_n_bytes(const char* path, int32_t n);

void decode_init();
void decode_bc1(uint8_t* dest, uint8_t* src, int32_t width, int32_t height);
void decode_bc7(uint8_t* dest, uint8_t* src, int32_t width, int32_t height);
void unswizzle(uint8_t* dest, uint8_t* src, int32_t width, int32_t height, const int32_t* swizzle_table);
void write_png(const char* filename, const unsigned char* image, unsigned w, unsigned h);

int main(int argc, char** argv)
{
	if(argc < 2) {
		fprintf(stderr, "usage: %s <.texture file>\n", argv[0]);
		return 1;
	}
	
	if(strcmp(argv[1], "run_bc1_test") == 0) {
		uint8_t* test = load_last_n_bytes("data/test.dds", (512*512)/2);
		uint8_t* dec = malloc(512 * 512 * 4);
		decode_bc1(dec, test, 512, 512);
		write_png("test_output.png", dec, 512, 512);
		return 0;
	}
	
	// Parse the arguments, determine the paths of the input and output files.
	const char* texture_file = argv[1];
	
	char* stream_file = malloc(strlen(texture_file) + strlen(".stream") + 1);
	strcpy(stream_file, texture_file);
	memcpy(stream_file + strlen(texture_file), ".stream", strlen(".stream") + 1);
	
	char* output_file = malloc(strlen(texture_file) + strlen(".png") + 1);
	strcpy(output_file, texture_file);
	memcpy(output_file + strlen(output_file), ".png", strlen(".png") + 1);
	
	// Parse the container format.
	DatFile dat = parse_dat_file(texture_file);
	
	printf("asset type %x\n", dat.asset_type_hash);
	
	for(int32_t i = 0; i < dat.section_count; i++) {
		printf("section of type %x @ %x\n", dat.sections[i].type_hash, dat.sections[i].offset);
	}
	
	verify(dat.section_count > 0 && dat.sections[0].type_hash == 0x4ede3593, "Bad sections.");
	TextureHeader* tex_header = (TextureHeader*) dat.sections[0].data;
	
	verify(tex_header->width % 256 == 0, "error: Texture width not a multiple of 256.");
	verify(tex_header->height % 256 == 0, "error: Texture height not a multiple of 256.");
	
	printf("width: %hd\n", tex_header->width);
	printf("height: %hd\n", tex_header->height);
	printf("type: %hhx\n", tex_header->type);
	
	int32_t pixel_count = tex_header->width * tex_header->height;
	
	int32_t texture_size;
	int tex_type;
	switch(tex_header->type) {
		case 0x47:
		case 0x48: {
			texture_size = pixel_count / 2;
			tex_type = TEX_BC1;
			break;
		}
		case 0x62:
		case 0x63: {
			texture_size = pixel_count;
			tex_type = TEX_BC7;
			break;
		}
		default: {
			printf("error: Bad texture type!\n");
			exit(1);
		}
	}
	
	// Read the highest mip from disk.
	uint8_t* compressed = load_last_n_bytes(stream_file, texture_size);
	
	// Allocate memory for decompression and unswizzling.
	uint8_t* decompressed = malloc(tex_header->width * tex_header->height * 4);
	uint8_t* unswizzled = malloc(tex_header->width * tex_header->height * 4);
	
	// Decompress and unswizzle the textures.
	decode_init();
	switch(tex_type) {
		case TEX_BC1: {
			decode_bc1(decompressed, compressed, tex_header->width, tex_header->height);
			unswizzle(unswizzled, decompressed, tex_header->width, tex_header->height, BC1_SWIZZLE_TABLE);
			break;
		}
		case TEX_BC7: {
			decode_bc7(decompressed, compressed, tex_header->width, tex_header->height);
			unswizzle(unswizzled, decompressed, tex_header->width, tex_header->height, BC7_SWIZZLE_TABLE);
			break;
		}
	}
	
	free(compressed);
	free(decompressed);
	
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
		fprintf(stderr, "error: Failed to open '%s' for reading.", path);
		exit(1);
	}
	fseek(file, 0, SEEK_END);
	fseek(file, -n, SEEK_CUR);
	printf("mip @ %lx\n", ftell(file));
	uint8_t* buffer = malloc(n);
	check_fread(fread(buffer, n, 1, file));
	fclose(file);
	return buffer;
}
