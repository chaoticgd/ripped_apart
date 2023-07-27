#include "archive.h"

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
	
	archive->entries = malloc(header.file_count * sizeof(RA_ArchiveEntry));
	archive->entry_count = header.file_count;
	if(archive->entries == NULL) {
		return "malloc";
	}
	
	if(fread(archive->entries, sizeof(RA_ArchiveEntry), archive->entry_count, archive->file) != archive->entry_count) {
		free(archive->entries);
		return "fread entires";
	}
	
	return RA_SUCCESS;
}

RA_Result RA_archive_close(RA_Archive* archive) {
	fclose(archive->file);
	free(archive->entries);
	return RA_SUCCESS;
}

RA_Result RA_archive_read(RA_Archive* archive, u32 index, u8** data_dest, u32* size_dest) {
	if(fseek(archive->file, archive->entries[index].offset, SEEK_SET) != 0) {
		return "fseek";
	}
	*data_dest = malloc(archive->entries[index].compressed_size);
	*size_dest = archive->entries[index].compressed_size;
	if(fread(*data_dest, *size_dest, 1, archive->file) != 1) {
		free(*data_dest);
		return "fread";
	}
	return RA_SUCCESS;
}
