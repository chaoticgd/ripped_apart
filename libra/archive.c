#include "archive.h"

#include <lz4.h>

static RA_Result load_block(RA_Archive* archive, RA_ArchiveBlock* block);

RA_Result RA_archive_open(RA_Archive* archive, const char* path) {
	memset(archive, 0, sizeof(RA_Archive));
	archive->file = fopen(path, "rb");
	if(!archive->file) {
		return "fopen";
	}
	
	RA_ArchiveHeader header;
	if(fread(&header, sizeof(RA_ArchiveHeader), 1, archive->file) != 1) {
		return "fread header";
	}
	
	archive->blocks = calloc(header.block_count, sizeof(RA_ArchiveBlock));
	archive->block_count = header.block_count;
	if(archive->blocks == NULL) {
		return "malloc";
	}
	
	for(u32 i = 0; i < archive->block_count; i++) {
		if(fread(&archive->blocks[i].header, sizeof(RA_ArchiveBlockHeader), 1, archive->file) != 1) {
			free(archive->blocks);
			return "fread block header";
		}
	}
	
	return RA_SUCCESS;
}

RA_Result RA_archive_close(RA_Archive* archive) {
	fclose(archive->file);
	for(u32 i = 0; i < archive->block_count; i++) {
		if(archive->blocks[i].decompressed_data != NULL) {
			free(archive->blocks[i].decompressed_data);
		}
	}
	free(archive->blocks);
	return RA_SUCCESS;
}

RA_Result RA_archive_read_decompressed(RA_Archive* archive, u8* data_dest, u32 decompressed_offset, u32 decompressed_size) {
	for(u32 i = 0; i < archive->block_count; i++) {
		RA_ArchiveBlock* block = &archive->blocks[i];
		b8 should_be_loaded =
			block->header.decompressed_offset < (decompressed_offset + decompressed_size) &&
			block->header.decompressed_offset + block->header.decompressed_size > decompressed_offset;
		if(should_be_loaded) {
			if(block->decompressed_data == NULL) {
				load_block(archive, block);
			}
			s64 src_offset = decompressed_offset - block->header.decompressed_offset;
			s64 dest_offset = block->header.decompressed_offset - decompressed_offset;
			s64 copy_size = decompressed_size;
			if(src_offset < 0) {
				src_offset = 0;
				dest_offset += src_offset;
				copy_size -= src_offset;
			}
			if(dest_offset < 0) {
				dest_offset = 0;
				src_offset += dest_offset;
				copy_size -= dest_offset;
			}
			if(src_offset + copy_size > block->decompressed_size) {
				copy_size = block->decompressed_size - src_offset;
			}
			if(dest_offset + copy_size > decompressed_size) {
				copy_size = decompressed_size - src_offset;
			}
			memcpy(data_dest + dest_offset, block->decompressed_data + src_offset, copy_size);
		} else if(block->decompressed_data != NULL) {
			free(block->decompressed_data);
			block->decompressed_data = NULL;
			block->decompressed_size = 0;
		}
	}
}

static RA_Result load_block(RA_Archive* archive, RA_ArchiveBlock* block) {
	if(fseek(archive->file, block->header.compressed_offset, SEEK_SET) != 0) {
		return "fseek";
	}
	u8* compressed_data = malloc(block->header.compressed_size);
	u32 compressed_size = block->header.compressed_size;
	if(fread(compressed_data, compressed_size, 1, archive->file) != 1) {
		free(compressed_data);
		return "fread";
	}
	
	switch(block->header.compression_mode) {
		case RA_ARCHIVE_COMPRESSION_TEXTURE: {
			block->decompressed_size = 0;
			break;
		}
		case RA_ARCHIVE_COMPRESSION_LZ4: {
			block->decompressed_data = malloc(block->header.decompressed_size);
			s32 bytes_written = LZ4_decompress_safe((char*) compressed_data, (char*) block->decompressed_data, compressed_size, block->header.decompressed_size);
			if(bytes_written != block->header.decompressed_size) {
				free(compressed_data);
				free(block->decompressed_data);
				return "failed to load block";
			}
			block->decompressed_size = block->header.decompressed_size;
			break;
		}
		default: {
			free(compressed_data);
			return "unknown compression mode";
		}
	}
	
	free(compressed_data);
	
	return RA_SUCCESS;
}
