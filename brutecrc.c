#include <ctype.h>

#include "libra/util.h"

static void load_crcs(u32** crcs_dest, u32* crc_count_dest, const char* path);
static char** build_dictionary_index(u8* dictionary_data, u32 dictionary_size);
static s32 lookup_crc(u32* crcs, u32 crc_count, u32 target_crc);

int main(int argc, char** argv) {
	RA_Result result;
	
	if(argc != 4) {
		puts("usage: brutecrc <target crc list file> <first word> <dictionary file>");
		return 1;
	}
	
	const char* target_crc_list_string = argv[1];
	const char* first_word = argv[2];
	const char* dictionary_path = argv[3];
	
	u32* crcs;
	u32 crc_count;
	load_crcs(&crcs, &crc_count, target_crc_list_string);
	
	u32 top = strlen(first_word);
	
	char string[4096];
	RA_string_copy(string, first_word, sizeof(string));
	string[top] = ' ';
	top++;
	
	u8* dictionary_data;
	s64 dictionary_size;
	if((result = RA_file_read(dictionary_path, &dictionary_data, &dictionary_size)) != RA_SUCCESS) {
		fprintf(stderr, "error: Cannot open dictionary file '%s' (%s).\n", dictionary_path, result->message);
		return 1;
	}
	
	char** dictionary = build_dictionary_index(dictionary_data, (u32) dictionary_size);
	
	u32 word_count = 0;
	for(s32 i = 0; dictionary[i] != NULL; i++) {
		word_count++;
	}
	
	for(u32 i = 0; i < word_count; i++) {
		RA_string_copy(&string[top], dictionary[i], sizeof(string) - top);
		string[top] = toupper(string[top]);
		u64 word_size = strlen(dictionary[i]);
		top += word_size;
		string[top] = '\0';
		u32 crc = RA_crc32_string(string);
		s32 result = lookup_crc(crcs, crc_count, crc);
		if(result > -1) {
			printf("%x %s\n", crc, string);
		}
		
		string[top] = ' ';
		top++;
		
		//for(u32 j = 0; j < word_count; j++) {
		//	RA_string_copy(&string[top], dictionary[j], sizeof(string) - top);
		//	string[top] = toupper(string[top]);
		//	u64 word_size = strlen(dictionary[j]);
		//	top += word_size;
		//	string[top] = '\0';
		//	u32 crc = RA_crc32_string(string);
		//	s32 result = lookup_crc(crcs, crc_count, crc);
		//	if(result > -1) {
		//		printf("%x %s\n", crc, string);
		//	}
		//	top -= word_size;
		//}
		
		top--;
		top -= word_size;
	}
}

static void load_crcs(u32** crcs_dest, u32* crc_count_dest, const char* path) {
	u32* crcs = malloc(1024 * 1024);
	u32 crc_count = 0;
	
	*crcs_dest = crcs;
	
	FILE* crcs_file = fopen(path, "r");
	if(!crcs_file) {
		fprintf(stderr, "Failed to open crcs file.\n");
		exit(1);
	}
	char line[1024];
	while(fgets(line, 1024, crcs_file)) {
		sscanf(line, "%08x", crcs);
		crcs++;
		crc_count++;
	}
	fclose(crcs_file);
	*crc_count_dest = crc_count;
}

static char** build_dictionary_index(u8* dictionary_data, u32 dictionary_size) {
	char** index = calloc(dictionary_size, 8);
	char* word = (char*) dictionary_data;
	char* ptr = (char*) dictionary_data;
	char* end = (char*) dictionary_data + dictionary_size;
	s32 i = 0;
	for(; ptr < end;) {
		if(*ptr == '\n') {
			index[i++] = word;
			*ptr = '\0';
			word = ++ptr;
		} else {
			ptr++;
		}
	}
	return index;
}

static s32 lookup_crc(u32* crcs, u32 crc_count, u32 target_crc) {
	if(crc_count == 0) {
		return -1;
	}
	
	s64 first = 0;
	s64 last = crc_count - 1;
	
	while(first <= last) {
		s64 mid = (first + last) / 2;
		if(crcs[mid] < target_crc) {
			first = mid + 1;
		} else if(crcs[mid] > target_crc) {
			last = mid - 1;
		} else {
			return mid;
		}
	}
	
	return -1;
}
