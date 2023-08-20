#include "libra/dat_container.h"
#include "libra/dependency_dag.h"

int main(int argc, char** argv) {
	RA_Result result;
	
	if(argc != 3 && argc != 4) {
		fprintf(stderr, "usage: ./diffdat <left file> <right file> [bytes before header]\n");
		return 1;
	}
	
	u8* lhs_data;
	s64 lhs_size;
	if((result = RA_file_read(argv[1], &lhs_data, &lhs_size)) != RA_SUCCESS) {
		fprintf(stderr, "error: Failed to read left file (%s).\n", result->message);
		return 1;
	}
	
	u8* rhs_data;
	s64 rhs_size;
	if((result = RA_file_read(argv[2], &rhs_data, &rhs_size)) != RA_SUCCESS) {
		fprintf(stderr, "error: Failed to read right file (%s).\n", result->message);
		return 1;
	}
	
	if(argc == 4) {
		s32 bytes_before_header = strtol(argv[3], NULL, 10);
		if(bytes_before_header > lhs_size || bytes_before_header > rhs_size) {
			fprintf(stderr, "error: File too small to contain header.\n");
			return 1;
		}
		lhs_data += bytes_before_header;
		lhs_size -= bytes_before_header;
		rhs_data += bytes_before_header;
		rhs_size -= bytes_before_header;
	}
	
	if((result = RA_dat_test(lhs_data, lhs_size, rhs_data, (u32) rhs_size, true)) != RA_SUCCESS) {
		fprintf(stderr, "%s\n", result->message);
	}
}
