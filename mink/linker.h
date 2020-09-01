#include "mage.h"

#define DEFAULT_OUT_FILE "out.mage"
#define DEFAULT_ENTRY_SYMBOL "main"
#define MAX_SYM_LEN 64

typedef struct lib
{
	char* name;
	struct lib* next;
}lib;

typedef struct rel
{
	uint32_t vaddr;
	struct rel* next;
}rel;

typedef struct sym
{
	char* name;
	uint32_t value;
	uint8_t class;
	rel* relocs;
	struct sym* next;
}sym;

typedef struct obj
{
	uint8_t* code;
	uint32_t csize;
	sym* symbols;
	struct obj* next;
}obj;

void readObj(obj** objList, char* file);
obj* parseCOFF(uint8_t* buf, long size);
obj* parseELF(uint8_t* buf, long size);
uint16_t link(void** bin, obj* objList, lib* libList, char* entrySym);

#ifdef WIN32
		obj*(*parse)(uint8_t* buf, long size) = parseCOFF;
#elif UNIX
		obj*(*parse)(uint8_t* buf, long size) = parseELF;
#endif