#include "archive.h"

RA_Result RA_archive_read_entries(RA_Archive* archive, const char* path) {
	FILE* file = fopen(path, "rb");
	if(!file) {
		return "fopen";
	}
	
	RA_ArchiveHeader header;
	if(fread(&header, sizeof(RA_ArchiveHeader), 1, file) != 1) {
		return "fread header";
	}
	
	memset(archive, 0, sizeof(RA_Archive));
	archive->files = malloc(header.file_count * sizeof(RA_ArchiveEntry));
	archive->file_count = header.file_count;
	if(archive->files == NULL) {
		return "malloc";
	}
	
	if(fread(archive->files, sizeof(RA_ArchiveEntry), archive->file_count, file) != archive->file_count) {
		free(archive->files);
		return "fread entires";
	}
	
	return RA_SUCCESS;
}

RA_Result RA_archive_free(RA_Archive* archive) {
	free(archive->files);
	return RA_SUCCESS;
}

RA_Result RA_archive_read_file(RA_Archive* archive, u32 index, u8** data_dest, u32* size_dest) {
	return RA_SUCCESS;
}
