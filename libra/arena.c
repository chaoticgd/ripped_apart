#include "arena.h"

void RA_arena_create(RA_Arena* arena) {
	memset(arena, 0, sizeof(RA_Arena));
}

void RA_arena_destroy(RA_Arena* arena) {
	RA_ArenaBlock* block = arena->head;
	while(block != NULL) {
		RA_ArenaBlock* next = block->next;
		free(block);
		block = next;
	}
}

void* RA_arena_alloc_aligned(RA_Arena* arena, s64 size, s64 alignment) {
	u64 offset = ALIGN(arena->top, alignment);
	if(!arena->head) {
		s64 capacity = MAX(size, 1024);
		arena->head = malloc(sizeof(RA_ArenaBlock) + capacity);
		memset(arena->head, 0, sizeof(RA_ArenaBlock));
		arena->head->capacity = capacity;
		arena->tail = arena->head;
		offset = 0;
	} else if(offset + size > arena->tail->capacity) {
		RA_ArenaBlock* prev = arena->tail;
		s64 capacity = MAX(size, prev->capacity * 2);
		arena->tail = malloc(sizeof(RA_ArenaBlock) + capacity);
		memset(arena->tail, 0, sizeof(RA_ArenaBlock));
		arena->tail->prev = prev;
		arena->tail->capacity = capacity;
		prev->next = arena->tail;
		offset = 0;
	}
	arena->top = offset + size;
	arena->tail->top = offset + size;
	return arena->tail->data + offset;
}

void* RA_arena_alloc(RA_Arena* arena, s64 size) {
	return RA_arena_alloc_aligned(arena, size, 8);
}

void* RA_arena_calloc(RA_Arena* arena, s64 element_count, s64 element_size) {
	s64 allocation_size = element_count * element_size;
	void* ptr = RA_arena_alloc(arena, allocation_size);
	memset(ptr, 0, allocation_size);
	return ptr;
}

s64 RA_arena_copy(RA_Arena* arena, u8* data_dest, s64 max_size) {
	s64 bytes_copied = 0;
	RA_ArenaBlock* block = arena->head;
	while(block != NULL) {
		if(block->top > max_size) {
			fprintf(stderr, "RA_arena_copy: Copy failed, output buffer too small.\n");
			abort();
		}
		memcpy(data_dest, block->data, block->top);
		bytes_copied += block->top;
		data_dest += block->top;
		max_size -= block->top;
		block = block->next;
	}
	return bytes_copied;
}
