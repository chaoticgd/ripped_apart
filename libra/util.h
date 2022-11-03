#include <stdint.h>

#define verify(cond, msg) if(!(cond)) { printf(msg "\n"); exit(1); }
#define check_fread(num) if((num) != 1) { printf("error: fread failed (line %d)\n", __LINE__); exit(1); }
#define check_fwrite(num) if((num) != 1) { printf("error: fwrite failed (line %d)\n", __LINE__); exit(1); }
#define check_fputc(num) if((num) == EOF) { printf("error: fputc failed (line %d)\n", __LINE__); exit(1); }
#define check_fputs(num) if((num) == EOF) { printf("error: fputs failed (line %d)\n", __LINE__); exit(1); }
#define check_fseek(num) if((num) != 0) { printf("error: fseek failed (line %d)\n", __LINE__); exit(1); }
typedef const char* RA_Result;
