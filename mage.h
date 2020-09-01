#ifdef WIN32
#include <Windows.h>
#include <conio.h>
#elif UNIX
#include <unistd.h>
#include <elf.h>
#endif

#include <stdio.h>
#include <stdint.h>

#define MAGE_MAGIC 0x4547414d
#define MAGE(x) ((MAGE_HEADER*)x)

#pragma pack(1)

typedef struct MAGE_HEADER
{
	uint32_t magic;
	uint32_t entry;
	uint32_t csize;
	uint16_t rptr;
	uint16_t cptr;
}MAGE_HEADER;

typedef uint32_t MAGE_RELOC_VADDR;
typedef uint16_t MAGE_STR_PTR;
typedef MAGE_STR_PTR MAGE_IMPORT_ENTRY;

typedef struct MAGE_RELOC_ENTRY
{
	MAGE_STR_PTR strptr;
	MAGE_RELOC_VADDR vaddr;
}MAGE_RELOC_ENTRY;