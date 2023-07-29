#include "../libra/util.h"

int main(int argc, char** argv) {
	if(argc != 2) {
		fprintf(stderr, "usage: ./igcrc \"string to crc\"");
		return 1;
	}
	printf("%08x\n", RA_crc_string(argv[1]));
	return 0;
}
