#include <memory>
#include <string>
#include <map>
#include <iostream>
#include <sstream>
#include <inttypes.h>
#include <Zydis/Zydis.h>
#include <../src/Generated/EnumRegister.inc>
#include "mage.h"

using namespace std;

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

enum class leaf
{
	none,
	raw,
	reg,
	var
};

struct expr
{
	shared_ptr<expr> left = nullptr;
	shared_ptr<expr> right = nullptr;
	leaf type = leaf::none;
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
		value = 0;
	}
};

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
	uint64_t value;
	shared_ptr<var> next = nullptr;
};

struct func
{
	shared_ptr<icode> code = nullptr;
	string name;
	uint8_t cConv = NULL;

	shared_ptr<var> getVar(uint64_t value, varType type)
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

	void addVar(uint64_t value, varType type)
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

extern ZydisDecoder decoder;
extern ZydisFormatter formatter;
extern map<uint64_t, shared_ptr<func>> functions;
extern map<uint64_t, shared_ptr<icode>> instructions;
extern uint64_t baseAddress;
extern uint32_t codeSize;
extern void* bin;

void parse(shared_ptr<icode> tail, shared_ptr<expr> registers[ZYDIS_REGISTER_MAX_VALUE], uint64_t address, shared_ptr<var> args);
string decompile(shared_ptr<func> func);