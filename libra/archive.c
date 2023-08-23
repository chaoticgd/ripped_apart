#include "archive.h"

#include <lz4.h>
#include "gdeflate_wrapper.h"

static RA_Result load_dsar_block(RA_Archive* archive, RA_ArchiveBlock* block);

RA_Result RA_archive_open(RA_Archive* archive, const char* path) {
	memset(archive, 0, sizeof(RA_Archive));
	archive->file = fopen(path, "rb");
	if(!archive->file) {
		return RA_FAILURE("fopen");
	}
	
	RA_ArchiveHeader header;
	if(fread(&header, sizeof(RA_ArchiveHeader), 1, archive->file) != 1) {
		return RA_FAILURE("fread header");
	}
	
	if(header.magic == FOURCC("DSAR")) {
		archive->is_dsar_archive = true;
		
		archive->dsar_blocks = calloc(header.block_count, sizeof(RA_ArchiveBlock));
		archive->dsar_block_count = header.block_count;
		if(archive->dsar_blocks == NULL) {
			return RA_FAILURE("malloc");
		}
		
		for(u32 i = 0; i < archive->dsar_block_count; i++) {
			if(fread(&archive->dsar_blocks[i].header, sizeof(RA_ArchiveBlockHeader), 1, archive->file) != 1) {
				free(archive->dsar_blocks);
				return RA_FAILURE("fread block header");
			}
		}
	} else {
		archive->is_dsar_archive = false;
	}
	
	return RA_SUCCESS;
}

RA_Result RA_archive_close(RA_Archive* archive) {
	fclose(archive->file);
	if(archive->is_dsar_archive) {
		for(u32 i = 0; i < archive->dsar_block_count; i++) {
			if(archive->dsar_blocks[i].decompressed_data != NULL) {
				free(archive->dsar_blocks[i].decompressed_data);
			}
		}
		free(archive->dsar_blocks);
	}
	return RA_SUCCESS;
}

s64 RA_archive_get_decompressed_size(RA_Archive* archive) {
	if(archive->is_dsar_archive) {
		if(archive->dsar_block_count == 0) {
			return 0;
		} else {
			RA_ArchiveBlock* block = &archive->dsar_blocks[archive->dsar_block_count - 1];
			return block->header.decompressed_offset + block->header.decompressed_size;
		}
	} else {
		return RA_file_size(archive->file);
	}
}

RA_Result RA_archive_read(RA_Archive* archive, u32 offset, u32 size, u8* data_dest) {
	RA_Result result;
	
	if(archive->is_dsar_archive) {
		for(u32 i = 0; i < archive->dsar_block_count; i++) {
			RA_ArchiveBlock* block = &archive->dsar_blocks[i];
			b8 should_be_loaded =
				block->header.decompressed_offset < (offset + size) &&
				block->header.decompressed_offset + block->header.decompressed_size > offset;
			if(should_be_loaded) {
				if(block->decompressed_data == NULL && (result = load_dsar_block(archive, block)) != RA_SUCCESS) {
					return result;
				}
				
				s64 begin = MAX(block->header.decompressed_offset, offset);
				s64 block_end = block->header.decompressed_offset + block->decompressed_size;
				s64 file_end = offset + size;
				s64 end = MIN(block_end, file_end);
				
				s64 dest_offset = begin - offset;
				s64 src_offset = begin - block->header.decompressed_offset;
				s64 copy_size = end - begin;
				
				memcpy(data_dest + dest_offset, block->decompressed_data + src_offset, copy_size);
			} else if(block->decompressed_data != NULL) {
				free(block->decompressed_data);
				block->decompressed_data = NULL;
				block->decompressed_size = 0;
			}
		}
	} else {
		if(fseek(archive->file, offset, SEEK_SET) != 0) {
			return RA_FAILURE("cannot seek to asset");
		}
		if(fread(data_dest, size, 1, archive->file) != 1) {
			return RA_FAILURE("cannot read asset");
		}
	}
	
	return RA_SUCCESS;
}

static RA_Result load_dsar_block(RA_Archive* archive, RA_ArchiveBlock* block) {
	if(fseek(archive->file, block->header.compressed_offset, SEEK_SET) != 0) {
		return RA_FAILURE("cannot seek to block");
	}
	u8* compressed_data = malloc(block->header.compressed_size);
	u32 compressed_size = block->header.compressed_size;
	if(compressed_data == NULL) {
		return RA_FAILURE("cannot allocate memory for compressed block");
	}
	if(fread(compressed_data, compressed_size, 1, archive->file) != 1) {
		free(compressed_data);
		return RA_FAILURE("cannot read block");
	}
	
	switch(block->header.compression_mode) {
		case RA_ARCHIVE_COMPRESSION_GDEFLATE: {
			block->decompressed_data = malloc(block->header.decompressed_size);
			if(block->decompressed_data == NULL) {
				free(compressed_data);
				return RA_FAILURE("cannot allocate memory for decompressed blcok");
			}
			if(!gdeflate_decompress(block->decompressed_data, block->header.decompressed_size, compressed_data, compressed_size, 8)) {
				free(compressed_data);
				free(block->decompressed_data);
				return RA_FAILURE("failed to decompress gdeflate block");
			}
			block->decompressed_size = block->header.decompressed_size;
			break;
		}
		case RA_ARCHIVE_COMPRESSION_LZ4: {
			block->decompressed_data = malloc(block->header.decompressed_size);
			if(block->decompressed_data == NULL) {
				free(compressed_data);
				return RA_FAILURE("cannot allocate memory for decompressed blcok");
			}
			s32 bytes_written = LZ4_decompress_safe((char*) compressed_data, (char*) block->decompressed_data, compressed_size, block->header.decompressed_size);
			if(bytes_written != block->header.decompressed_size) {
				free(compressed_data);
				free(block->decompressed_data);
				return RA_FAILURE("failed to decompress lz4 block");
			}
			block->decompressed_size = block->header.decompressed_size;
			break;
		}
		default: {
			free(compressed_data);
			return RA_FAILURE("unknown compression mode %hhd", block->header.compression_mode);
		}
	}
	
	free(compressed_data);
	
	return RA_SUCCESS;
}
