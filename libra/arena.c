#include "arena.h"

void RA_arena_create(RA_Arena* arena) {
	memset(arena, 0, sizeof(RA_Arena));
}

void RA_arena_destroy(RA_Arena* arena) {
	RA_ArenaBlock* block = arena->head;
	while(block != NULL) {
		RA_ArenaBlock* prev = block->prev;
		free(block);
		block = prev;
	}
}

void* RA_arena_alloc(RA_Arena* arena, s64 size) {
	u64 offset = arena->top + (-arena->top & 7);
	if(!arena->head) {
		s64 capacity = MAX(size, 1024);
		arena->head = malloc(sizeof(RA_ArenaBlock) + capacity);
		memset(arena->head, 0, sizeof(RA_ArenaBlock));
		arena->head->size = capacity;
		arena->top = 0;
	} else if(offset + size > arena->head->size) {
		RA_ArenaBlock* prev = arena->head;
		s64 capacity = MAX(size, prev->size * 2);
		arena->head = malloc(sizeof(RA_ArenaBlock) + capacity);
		memset(arena->head, 0, sizeof(RA_ArenaBlock));
		arena->head->prev = prev;
		arena->head->size = capacity;
		arena->top = 0;
	}
	arena->top = offset + size;
	return arena->head->data + offset;
}
