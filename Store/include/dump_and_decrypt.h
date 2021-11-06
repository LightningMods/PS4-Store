#include <stdbool.h>
#define SELF_MAGIC	0x1D3D154F
#define ELF_MAGIC	0x464C457F

int decrypt_and_dump_self(char* selfFile, char* saveFile) ;
bool is_self(const char* fn);