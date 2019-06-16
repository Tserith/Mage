#include "mage.h"

#define DEFAULT_OUT_FILE "out.mage"
#define DEFAULT_STACK_SIZE 256
#define DEFAULT_ENTRY_SYMBOL "start"
#define STACK_SIZE_SYMBOL "stack"
#define MAX_SYM_LEN 64

typedef struct str
{
	char* name;
	struct str* next;
}str;

typedef struct sym
{
	char* name;
	uint32_t value;
	uint8_t class;
	enum {NOREL, REL} type;
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
uint16_t link(void** bin, obj* objList, str* libList, char* entrySym);

#ifdef WIN32
		obj*(*parse)(uint8_t* buf, long size) = parseCOFF;
#elif UNIX
		obj*(*parse)(uint8_t* buf, long size) = parseELF;
#endif