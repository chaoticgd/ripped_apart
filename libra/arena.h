#ifndef LIBRA_ARENA_H
#define LIBRA_ARENA_H

#include "util.h"

typedef struct t_RA_ArenaBlock {
	struct t_RA_ArenaBlock* prev;
	u64 size;
	u8 data[];
} RA_ArenaBlock;

typedef struct {
	RA_ArenaBlock* head;
	u64 top;
} RA_Arena;

void RA_arena_create(RA_Arena* arena);
void RA_arena_destroy(RA_Arena* arena);
void* RA_arena_alloc(RA_Arena* arena, s64 size);
void* RA_arena_calloc(RA_Arena* arena, s64 element_count, s64 element_size);

#endif
