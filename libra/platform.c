#include "platform.h"

#include <dirent.h>
#include <stdarg.h>

#include "arena.h"

#ifdef WIN32
	#include <windows.h>
#include <shellapi.h>
	#include <io.h>
#define popen _popen
#define pclose _pclose
#define setenv(name, value, overwrite) (_putenv_s(name, value) == 0 ? 0 : -1)
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

void RA_open_file_path_or_url(const char* path_or_url) {
#ifdef _WIN32
	ShellExecuteA(nullptr, "open", path_or_url, nullptr, nullptr, SW_SHOWDEFAULT);
#else
	setenv("WRENCH_ARG_0", "xdg-open", 1);
	setenv("WRENCH_ARG_1", path_or_url, 1);
	system("\"$WRENCH_ARG_0\" \"$WRENCH_ARG_1\"");
#endif
}

void RA_thread_sleep_ms(s32 milliseconds) {
	#ifdef WIN32
		Sleep(milliseconds);
	#else
		usleep(milliseconds);
	#endif
}

void RA_message_box(MessageBoxType type, const char* title, const char* format, ...) {
	va_list args;
	va_start(args, format);
	static char text[16 * 1024];
	vsnprintf(text, sizeof(text), format, args);
	va_end(args);
	
	puts(text);
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
