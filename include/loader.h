#include "mage.h"

void loadWindows(void* buf, long size, uint32_t vsize);
void loadLinux(void* buf, long size, uint32_t vsize);

#ifdef WIN32
		void(*load)(void*, long, uint32_t) = loadWindows;
#elif UNIX
		void(*load)(void*, long, uint32_t) = loadLinux;
#endif