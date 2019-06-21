#include "loader.h"

#ifdef WIN32
	int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
#elif(UNIX)
	int main(int argc, char* argv[])
#endif()
{
	MAGE_HEADER* buf;
	long fsize;
	
#ifdef WIN32
	int argc = __argc;
	char** argv = __argv;
#endif

	if ((unsigned int)argc < 2) error("No program provided to load", NULL);

	// read in executable to load
	FILE* fd = fopen(argv[1], "rb");
	if (!fd) error("Unable to open provided file", argv[1]);

	fseek(fd, 0, SEEK_END);
	fsize = ftell(fd);
	rewind(fd);

	buf = (MAGE_HEADER*)malloc(fsize);
	fread(buf, 1, fsize, fd);
	fclose(fd);

	if (fsize >= 4 && buf->magic != MAGE_MAGIC) error("Invalid signature", NULL);

	if (fsize < sizeof(MAGE_HEADER)) error("Header incomplete", NULL);

	if (buf->csize > fsize - sizeof(MAGE_HEADER)) error("Code size must not exceed size of file", NULL);

	if (buf->entry > buf->csize) error("Entry point must not exceed size of memory", NULL);

	if (buf->rptr > fsize || buf->cptr > fsize) error("Header pointers must not exceed size of file", NULL);

	// dynamically link imports
	dlink(buf);

	// load program into memory
	void* code = load(buf);

	free(buf);

	// set stack pointer to jmp - buf->entry + buf->csize

	// execute program
	(*(void(*)())(code))();

	VirtualFree(code, 0, MEM_RELEASE);

	return 0;
}

void dlink(void* buf)
{
	MAGE_HEADER* info = (MAGE_HEADER*)buf;
	MAGE_IMPORT_ENTRY* imports = (MAGE_IMPORT_ENTRY*)((uint8_t*)buf + sizeof(MAGE_HEADER));
	MAGE_RELOC_ENTRY* relocs = (MAGE_RELOC_ENTRY*)((uint8_t*)buf + info->rptr);

	// handle imports
	while ((uint8_t*)imports < (uint8_t*)buf + info->rptr)
	{
#ifdef WIN32
		HMODULE libHandle = LoadLibrary((LPCSTR)buf + *imports);
#elif UNIX
		// todo
#endif
		if (!libHandle) error("Unable to load library", (char*)buf + *imports);

		// fix relocations if found in library
		// first string pointer assumed to be beginning of string table
		while ((uint8_t*)relocs < (uint8_t*)buf + *(MAGE_STR_PTR*)((uint8_t*)buf + sizeof(MAGE_HEADER)))
		{
#ifdef WIN32
			FARPROC faddr = GetProcAddress(libHandle, (LPCSTR)buf + relocs->strptr);
#elif UNIX
			// todo
#endif
			if (faddr)
			{
				*(uint64_t*)((uint8_t*)buf + info->cptr + relocs->vaddr) = (uint64_t)faddr;
				relocs->strptr = 0;
			}
			
			relocs++;
		}
		
		relocs = (MAGE_RELOC_ENTRY*)((uint8_t*)buf + info->rptr);
		imports++;
	}

	// check for unlinked relocations
	// first string pointer assumed to be beginning of string table
	while ((uint8_t*)relocs < (uint8_t*)buf + *(MAGE_STR_PTR*)((uint8_t*)buf + sizeof(MAGE_HEADER)))
	{
		if (relocs->strptr) error("Unable to link function", (uint8_t*)buf + relocs->strptr);

		relocs++;
	}
}

void* load(void* buf)
{
	MAGE_HEADER* info = (MAGE_HEADER*)buf;

#ifdef WIN32
	LPVOID vMem = VirtualAlloc(NULL, info->csize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#elif UNIX
	// todo
#endif
	if (!vMem) error("[-] Unable to allocate virtual memory", NULL);

	memcpy((uint8_t*)vMem, (uint8_t*)buf + info->cptr, info->csize);
	
	return (uint8_t*)vMem + info->entry;
}

void error(const char* msg, const char* name)
{
#ifdef WIN32
	if (AllocConsole()) // used to display errors using windows subsystem
	{
		FILE* fpstdout = stdin;
		freopen_s(&fpstdout, "CONOUT$", "w", stdout);
	}
#elif UNIX
	// todo
#endif

	if (name) printf("[-] %s: %s\n", msg, name);
	else printf("[-] %s\n", msg);
	printf("\nPress any key to exit.\n");

#ifdef WIN32
	_getch();
#endif

	exit(1);
}