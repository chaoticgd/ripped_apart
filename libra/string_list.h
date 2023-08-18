#ifndef LIBRA_STRING_LIST_H
#define LIBRA_STRING_LIST_H

#include "arena.h"

typedef struct {
	RA_Arena arena;
	const char** strings;
	u32 count;
} RA_StringList;

// Call these functions in order.
void RA_string_list_create(RA_StringList* string_list);
RA_Result RA_string_list_add(RA_StringList* string_list, const char* string);
RA_Result RA_string_list_finish(RA_StringList* string_list);
void RA_string_list_destroy(RA_StringList* string_list);

#endif
