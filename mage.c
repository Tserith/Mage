#include "mage.h"

int main(int argc, char* argv[])
{
	void* buf;
	long fsize;

	if ((unsigned int)argc < 2)
	{
		printf("[-] No program provided to load\n");
		exit(1);
	}

	// read in executable to load
	FILE* fd = fopen(argv[1], "rb");
	if (!fd)
	{
		printf("[-] Unable to open provided file\n");
		exit(1);
	}

	fseek(fd, 0, SEEK_END);
	fsize = ftell(fd);
	rewind(fd);

	buf = malloc(fsize);
	fread(buf, 1, fsize, fd);
	fclose(fd);

	if (fsize >= 4 && ((MAGE_HEADER*)buf)->magic != 0x4547414d)
	{
		printf("[-] Invalid signature\n");
		exit(1);
	}

	if (fsize < sizeof(MAGE_HEADER))
	{
		printf("[-] Header incomplete\n");
		exit(1);
	}

	if (((MAGE_HEADER*)buf)->entry > ((MAGE_HEADER*)buf)->vsize)
	{
		printf("[-] Entry point must not exceed size of memory\n");
		exit(1);
	}

	// load program into memory
	load(buf, fsize - sizeof(MAGE_HEADER), ((MAGE_HEADER*)buf)->vsize);

	return 0;
}

void loadWindows(void* buf, long size, uint32_t vsize)
{
	LPVOID vMem = VirtualAlloc(NULL, vsize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!vMem)
	{
		printf("[-] Unable to allocate virtual memory\n");
		exit(1);
	}

	memcpy(vMem, (uint8_t*)buf + sizeof(MAGE_HEADER), size);
	(uint64_t)vMem += ((MAGE_HEADER*)buf)->entry;
	free(buf);

	// execute program
	(*(void(*)())(vMem))();

	VirtualFree(vMem, 0, MEM_RELEASE);
}

void loadLinux(void* buf, long size, uint32_t vsize)
{
	// https://stackoverflow.com/questions/37122186/c-put-x86-instructions-into-array-and-execute-them
}