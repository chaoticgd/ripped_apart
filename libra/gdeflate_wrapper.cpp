#include "gdeflate_wrapper.h"

#include <GDeflate.h>

size_t gdeflate_compress_bound(size_t size) {
	return GDeflate::CompressBound(size);
}

b8 gdeflate_compress(u8* output, size_t* output_size, const u8* in, size_t in_size, u32 level, u32 flags) {
	return GDeflate::Compress(output, output_size, in, in_size, level, flags);
}

b8 gdeflate_decompress(u8* output, size_t output_size, const u8* in, size_t in_size, u32 num_workers) {
	return GDeflate::Decompress(output, output_size, in, in_size, num_workers);
}
