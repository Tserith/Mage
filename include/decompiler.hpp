#include <memory>
#include <string>
#include <map>
#include <iostream>
#include <sstream>
#include <inttypes.h>
#include <Zydis/Zydis.h>
#include <../src/Generated/EnumRegister.inc>
#include "mage.h"

enum class itype
{
	none,
	assign,
	call,
	ret,
	add,
	jne,
	jeq
};

enum class optr
{
	none,
	mov,
	add,
	sub,
	mul,
	div,
	or,
	xor,
	and,
	not,
	shl,
	shr,
	deref,
	ref
};

enum class idtype
{
	none,
	reg,
	var
};

using namespace std;

// if left and right pointers are null (leaf) it's an operand, otherwise, an operator
struct expr
{
	shared_ptr<expr> left = nullptr;
	shared_ptr<expr> right = nullptr;
	idtype type = idtype::none;
	uint16_t id = 0;
	union
	{
		struct
		{
			optr type;
		}operation;
		uint64_t value;
	};

	expr()
	{
		operation.type = optr::none;
	}
};

struct icode
{
	shared_ptr<icode> branch = nullptr;
	shared_ptr<icode> next = nullptr;
	shared_ptr<expr> expr = nullptr;
	itype type = itype::none;
};

enum class varType
{
	none,
	stack,
	reg
};

struct var
{
	string name;
	varType type = varType::none;
	uint16_t value;
	shared_ptr<var> next = nullptr;
};

struct func
{
	shared_ptr<icode> code = nullptr;
	string name;
	uint8_t cConv = NULL;

	shared_ptr<var> getVar(uint16_t value, varType type)
	{
		for (auto temp = locals; temp; temp = temp->next)
		{
			if (temp->value == value && temp->type == type)
			{
				return temp;
			}
		}

		addVar(value, type);
		return locals;
	}
	
private:
	shared_ptr<var> locals = nullptr;

	void addVar(uint16_t value, varType type)
	{
		auto newVar = make_shared<var>();

		newVar->name = "var_";
		newVar->value = value;
		newVar->type = type;

		if (type == varType::stack)
		{
			stringstream stream;
			stream << hex << value;
			newVar->name += stream.str();
		}
		if (type == varType::reg)
		{
			newVar->name += string(STR_REGISTER[value].data);
		}

		newVar->next = locals;
		locals = newVar;
	}
};

void genIRx64(shared_ptr<icode> tail, shared_ptr<expr> registers[ZYDIS_REGISTER_MAX_VALUE], uint64_t address, shared_ptr<var> args);
string decompile(shared_ptr<func> func);
long readFileRaw(char* file, void** buf);