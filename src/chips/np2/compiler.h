#ifndef FASTCALL
#define FASTCALL
#endif
#define SOUNDCALL FASTCALL

#ifndef _WIN32
#include <stdint.h>
typedef uint32_t DWORD;
#endif

#ifndef MAX_PATH
#define MAX_PATH 256
#endif
