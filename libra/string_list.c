#include "string_list.h"

void RA_string_list_create(RA_StringList* string_list) {
	memset(string_list, 0, sizeof(RA_StringList));
	RA_arena_create(&string_list->arena);
}

RA_Result RA_string_list_add(RA_StringList* string_list, const char* string) {
	s64 string_size = strlen(string) + 1;
	char* string_alloc = RA_arena_alloc_aligned(&string_list->arena, string_size, 1);
	if(string_alloc == NULL) {
		return RA_FAILURE("cannot allocate memory");
	}
	memcpy(string_alloc, string, string_size);
	string_list->count++;
	return RA_SUCCESS;
}

RA_Result RA_string_list_finish(RA_StringList* string_list) {
	string_list->strings = RA_arena_alloc(&string_list->arena, string_list->count * sizeof(char*));
	if(string_list->strings == NULL) {
		return RA_FAILURE("cannot allocate memory");
	}
	RA_ArenaBlock* block = string_list->arena.head;
	u32 offset = 0;
	for(u32 i = 0; i < string_list->count; i++) {
		string_list->strings[i] = (char*) &block->data[offset];
		while(block->data[offset] != '\0') offset++; // Skip past the string.
		offset++; // Skip null terminator.
		if(offset >= block->top) {
			block = block->next;
		}
		if(block == NULL) {
			return RA_FAILURE("too few blocks");
		}
	}
	return RA_SUCCESS;
}

void RA_string_list_destroy(RA_StringList* string_list) {
	RA_arena_destroy(&string_list->arena);
}
