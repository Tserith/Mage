#include "mage.h"

typedef struct sym
{
	char* name;
	uint32_t value;
	struct sym* next;
}sym;

typedef struct rel
{
	uint32_t addr;
	char* name;
	uint32_t value;
	struct rel* next;
}rel;

typedef struct obj
{
	void* code;
	uint32_t csize;
	sym* symbols;
	rel* relocs;
	struct obj* next;
}obj;

obj* parseCOFF(uint8_t* buf, long size);
obj* parseELF(uint8_t* buf, long size);
void link(obj* objList);

#ifdef WIN32
		obj*(*parse)(uint8_t* buf, long size) = parseCOFF;
#elif UNIX
		obj*(*parse)(uint8_t* buf, long size) = parseELF;
#endif