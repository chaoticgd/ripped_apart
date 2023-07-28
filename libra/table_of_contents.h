#ifndef LIBRA_TABLE_OF_CONTENTS_H
#define LIBRA_TABLE_OF_CONTENTS_H

#include "util.h"

typedef struct {
	/* 0x00 */ char data[0x42];
} RA_TOCArchive;

typedef struct {
	RA_TOCArchive* archives;
	u32 archive_count;
} RA_TableOfContents;

RA_Result RA_toc_parse(RA_TableOfContents* toc, u8* data, u32 size);
RA_Result RA_toc_build(RA_TableOfContents* toc, u8** data_dest, u32* size_dest);

#endif
