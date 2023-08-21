#ifndef LIBRA_PLATFORM_H
#define LIBRA_PLATFORM_H

#include "util.h"
#include "string_list.h"

void RA_make_dir(const char* path);
b8 RA_file_exists(const char* path);
RA_Result RA_enumerate_directory(RA_StringList* file_names_dest, const char* dir_path);
void RA_open_file_path_or_url(const char* path_or_url);
void RA_thread_sleep_ms(s32 milliseconds);

typedef enum {
	GUI_MESSAGE_BOX_INFO,
	GUI_MESSAGE_BOX_ERROR
} MessageBoxType;

void RA_message_box(MessageBoxType type, const char* title, const char* format, ...);

#endif
