#ifndef LIBRA_PLATFORM_H
#define LIBRA_PLATFORM_H

#include "util.h"

void RA_make_dir(const char* path);
void RA_thread_sleep_ms(s32 milliseconds);

typedef enum {
	GUI_MESSAGE_BOX_INFO,
	GUI_MESSAGE_BOX_ERROR
} MessageBoxType;

void RA_message_box(const char* text, const char* title, MessageBoxType type);

#endif
