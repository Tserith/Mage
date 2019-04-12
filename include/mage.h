#ifdef WIN32
        #include <Windows.h>
#elif UNIX
        #include <unistd.h>
		#include <elf.h>
#endif

#include <stdio.h>
#include <stdint.h>

typedef struct MAGE_HEADER
{
	uint32_t magic;
	uint32_t entry;
	uint32_t ssize;
	uint32_t eptr;
	uint32_t iptr;
	uint32_t cptr;
	uint32_t csize;
}MAGE_HEADER;