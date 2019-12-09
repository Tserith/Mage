#include <memory>
#include <string>
#include <map>
#include <inttypes.h>
#include <Zydis/Zydis.h>
#include "mage.h"

#define ICODE_MOV 1
#define ICODE_CALL 2
#define ICODE_RET 3
#define ICODE_ADD 4
#define ICODE_JNE 5
#define ICODE_JEQ 6
// etc

#define EXPR_CONSTANT 1
#define EXPR_ADDRESS 2
#define EXPR_REGISTER 3
#define EXPR_UNINIT 4

#define EXPR_CHAR 0x10
#define EXPR_SHORT 0x20
#define EXPR_INTEGER 0x30
#define EXPR_LONG 0x40
#define EXPR_FLOAT 0x50
#define EXPR_DOUBLE 0x60
// ah register?

#define EXPR_UNSIGNED 0x80
#define EXPR_SIGNED 0x100

#define REG_RAX 1
#define REG_RBX 2
#define REG_RCX 3
#define REG_RDX 4

#define REGISTER_COUNT 4

using namespace std;

struct icode
{
	shared_ptr<icode> branch = nullptr;
	shared_ptr<icode> next = nullptr;
	shared_ptr<icode> expr = nullptr;
	uint8_t type = NULL;
};

struct expr
{
	shared_ptr<icode> left = nullptr;
	shared_ptr<icode> right = nullptr;
	uint64_t value = NULL;
	uint8_t type = NULL;
};

struct var
{
	string name;
	uint8_t type = NULL;
	shared_ptr<var> next = nullptr;
};

struct func
{
	shared_ptr<icode> code = nullptr;
	shared_ptr<var> locals = nullptr;
	string name;
	uint8_t cConv = NULL;
};

void genIRx64(shared_ptr<icode> tail, shared_ptr<expr> registers[REGISTER_COUNT], uint64_t address, shared_ptr<var> args);
string decompile(shared_ptr<icode> IR);
long readFileRaw(char* file, void** buf);