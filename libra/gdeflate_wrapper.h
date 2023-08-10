#ifndef LIBRA_GDEFLATE_WRAPPER_H
#define LIBRA_GDEFLATE_WRAPPER_H

#include "util.h"

#ifdef __cplusplus
extern "C" {
#endif

size_t gdeflate_compress_bound(size_t size);
b8 gdeflate_compress(u8* output, size_t* output_size, const u8* in, size_t in_size, u32 level, u32 flags);
b8 gdeflate_decompress(u8* output, size_t output_size, const u8* in, size_t in_size, u32 num_workers);

#ifdef __cplusplus
}
#endif

#endif
