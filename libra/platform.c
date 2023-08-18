#include "platform.h"

#include <dirent.h>

#include "arena.h"

#ifdef WIN32
	#include <windows.h>
	#include <io.h>
#else
	#include <unistd.h>
	#include <sys/stat.h>
#endif

void RA_make_dir(const char* path) {
	#ifdef WIN32
		_mkdir(path);
	#else
		mkdir(path, 0777);
	#endif
}

b8 RA_file_exists(const char* path) {
#ifdef WIN32
	return _access(path, 0) == 0;
#else
	return access(path, F_OK) == 0;
#endif
}

RA_Result RA_enumerate_directory(RA_StringList* file_names_dest, const char* dir_path) {
	RA_Result result;
	RA_string_list_create(file_names_dest);
	
	DIR* directory = opendir(dir_path);
	if(!directory) {
		return RA_FAILURE("failed to open directory '%s'", dir_path);
	}
	struct dirent* entry;
	while((entry = readdir(directory)) != NULL) {
		if(entry->d_type == DT_REG) {
			if((result = RA_string_list_add(file_names_dest, entry->d_name)) != RA_SUCCESS) {
				closedir(directory);
				RA_string_list_destroy(file_names_dest);
				return result;
			}
		}
	}
	closedir(directory);
	
	if((result = RA_string_list_finish(file_names_dest)) != RA_SUCCESS) {
		RA_string_list_destroy(file_names_dest);
		return result;
	}
	
	return RA_SUCCESS;
}

void RA_thread_sleep_ms(s32 milliseconds) {
	#ifdef WIN32
		Sleep(milliseconds);
	#else
		usleep(milliseconds);
	#endif
}

void RA_message_box(const char* text, const char* title, MessageBoxType type) {
	fprintf(stderr, "error: %s\n", text);
#ifdef WIN32
	MessageBoxA(NULL, text, title, MB_OK);
#else
	if(fork() == 0) {
		char zenity_type[32];
		switch(type) {
			case GUI_MESSAGE_BOX_INFO: RA_string_copy(zenity_type, "--info", sizeof(zenity_type)); break;
			case GUI_MESSAGE_BOX_ERROR: RA_string_copy(zenity_type, "--error", sizeof(zenity_type)); break;
		}
		
		char zenity_text[1024];
		snprintf(zenity_text, sizeof(zenity_text), "--text=%s", text);
		
		char zenity_title[1024];
		snprintf(zenity_title, sizeof(zenity_title), "--title=%s", title);
		
		char* args[] = {
			"/usr/bin/env",
			"zenity",
			zenity_type,
			zenity_text,
			zenity_title,
			NULL
		};
		execv(args[0], args);
	}
#endif
}
