#ifdef WIN32
        #include <Windows.h>
#elif UNIX
        #include <unistd.h>
#endif

#include <stdio.h>
#include <stdint.h>

typedef struct MAGE_HEADER
{
	uint32_t magic;
	uint32_t vsize;
	uint32_t entry;
}MAGE_HEADER;

void loadWindows(void* buf, long size, uint32_t vsize);
void loadLinux(void* buf, long size, uint32_t vsize);

#ifdef WIN32
		void(*load)(void*, long, uint32_t) = loadWindows;
#elif UNIX
		void(*load)(void*, long, uint32_t) = loadLinux;
#endif