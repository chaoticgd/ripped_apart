#ifndef LIBRA_ARENA_H
#define LIBRA_ARENA_H

#include "util.h"

typedef struct t_RA_ArenaBlock {
	struct t_RA_ArenaBlock* prev;
	struct t_RA_ArenaBlock* next;
	u64 top;
	u64 capacity;
	u8 data[];
} RA_ArenaBlock;

typedef struct {
	RA_ArenaBlock* head;
	RA_ArenaBlock* tail;
	u64 top;
} RA_Arena;

void RA_arena_create(RA_Arena* arena);
void RA_arena_destroy(RA_Arena* arena);
void* RA_arena_alloc_aligned(RA_Arena* arena, s64 size, s64 alignment);
void* RA_arena_alloc(RA_Arena* arena, s64 size);
void* RA_arena_calloc(RA_Arena* arena, s64 element_count, s64 element_size);
s64 RA_arena_copy(RA_Arena* arena, u8* data_dest, s64 max_size);

#endif
