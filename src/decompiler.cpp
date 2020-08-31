#include "decompiler.hpp"

map<uint64_t, shared_ptr<func>> functions;
map<uint64_t, shared_ptr<icode>> instructions;

ZydisDecoder decoder;
ZydisFormatter formatter;

uint64_t baseAddress = 0x007FFFFFFF400000;
char instrStr[256];
void* bin;
uint32_t codeSize = 0;

int main(int argc, char* argv[])
{
	shared_ptr<expr> registers[ZYDIS_REGISTER_MAX_VALUE] = { nullptr };
	uint64_t startAddress;
	auto ir = make_shared<icode>();

	printf("[Seer] A simple x64 Decompiler - Written by Tserith\n\n");

	if ((unsigned int)argc < 2)
	{
		printf("Usage: seer [INPUT PROGRAM]\n");
		printf("Accepts MAGE executables\n");
		exit(1);
	}

	// init
	ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_ADDRESS_WIDTH_64);
	ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_INTEL);

	// parse input file
	codeSize = readFileRaw(argv[1], &bin);

	if (MAGE(bin)->magic == MAGE_MAGIC && codeSize > sizeof(MAGE_HEADER))
	{
		bin = (uint8_t*)bin + MAGE(bin)->cptr;
		codeSize = MAGE(bin)->csize;
	}
	else
	{
		printf("[-] Executable format is not supported\n");
		exit(1);
	}

	startAddress = MAGE(bin)->entry - MAGE(bin)->cptr;

	auto newFunc = make_shared<func>();
	newFunc->code = ir;
	newFunc->name = "start";
	auto result = functions.insert({ baseAddress + startAddress, newFunc });

	// generate IR for each function (beginning with start)
	genIRx64(ir, registers, baseAddress, nullptr);

	// decompile
	string src = decompile(result.first->second);

	printf(src.c_str());
}

// maybe use for variables too?
shared_ptr<expr> getRegister(shared_ptr<expr> registers[ZYDIS_REGISTER_MAX_VALUE], uint64_t address, uint16_t id)
{
	if (id == ZYDIS_REGISTER_RIP)
	{
		auto addr = make_shared<expr>();

		addr->value = address;

		return addr;
	}

	auto temp = registers[id];
	if (!temp)
	{
		temp = make_shared<expr>();
	}

	temp->type = idtype::reg;
	temp->id = id;

	return temp;
}

void genIRx64(shared_ptr<icode> tail, shared_ptr<expr> registers[ZYDIS_REGISTER_MAX_VALUE], uint64_t address, shared_ptr<var> args)
{
	ZydisDecodedInstruction instr;

	if (args)
	{
		// handle args
	}

	// iterate through instructions until ret
	while (ZydisDecoderDecodeBuffer(&decoder, &((uint8_t*)bin)[address - baseAddress], codeSize, &instr))
	{
		auto itr = instructions.find(address);

		// search instructions for address and use it if found
		if (itr != instructions.end())
		{
			tail = itr->second;

			// if is jump handle local variables

			break;
		}

		// Format & print the binary instruction structure to human readable format
		ZydisFormatterFormatInstruction(&formatter, &instr, instrStr, sizeof(instrStr), address);

		printf("%016" PRIX64 "  %s\n", address, instrStr);

		// todo:
		// enumerate variables if uninitialized registers or ebp+x addresses are used

		// do not continue if ret is encountered
		if (instr.meta.category == ZYDIS_CATEGORY_RET)
		{
			tail->type = itype::ret;
			break;
		}

		auto operate = make_shared<expr>();

		// handle operator
		switch (instr.mnemonic)
		{
		case ZYDIS_MNEMONIC_JMP:
		case ZYDIS_MNEMONIC_POP:
		case ZYDIS_MNEMONIC_PUSH:
		case ZYDIS_MNEMONIC_CALL:
			break;
		case ZYDIS_MNEMONIC_LEA:
		case ZYDIS_MNEMONIC_MOVD:
		case ZYDIS_MNEMONIC_MOVQ:
		case ZYDIS_MNEMONIC_MOV:
			operate->operation.type = optr::mov;
			break;
		case ZYDIS_MNEMONIC_INC:
		case ZYDIS_MNEMONIC_ADD:
			operate->operation.type = optr::add;
			break;
		case ZYDIS_MNEMONIC_SUB:
			operate->operation.type = optr::sub;
			break;
		case ZYDIS_MNEMONIC_MUL:
			operate->operation.type = optr::mul;
			break;
		case ZYDIS_MNEMONIC_DIV:
			operate->operation.type = optr::div;
			break;
		case ZYDIS_MNEMONIC_AND:
			operate->operation.type = optr::and;
			break;
		case ZYDIS_MNEMONIC_OR:
			operate->operation.type = optr::or;
			break;
		case ZYDIS_MNEMONIC_XOR:
			operate->operation.type = optr::xor;
			break;
		case ZYDIS_MNEMONIC_NOT:
			operate->operation.type = optr::not;
			break;
		case ZYDIS_MNEMONIC_SHL:
			operate->operation.type = optr::shl;
			break;
		case ZYDIS_MNEMONIC_SHR:
			operate->operation.type = optr::shr;
			break;
		default:
			printf("Unhandled mnemonic: %i\n", instr.mnemonic);
		}

		if (operate->operation.type != optr::none)
		{
			std::shared_ptr<expr>* updateReg = nullptr;

			// handle operands
			for (auto operand = instr.operands; operand != &instr.operands[instr.operand_count]; operand++)
			{
				std::shared_ptr<expr> node = nullptr;

				// fill in operand details
				switch (operand->type)
				{
				case ZYDIS_OPERAND_TYPE_REGISTER:
				{
					updateReg = &registers[operand->reg.value];
					node = getRegister(registers, address + instr.length, operand->reg.value);

					break;
				}
				case ZYDIS_OPERAND_TYPE_MEMORY:
					node = getRegister(registers, address + instr.length, operand->mem.base);

					// segment/index, register, scale?

					if (operand->mem.disp.has_displacement)
					{
						auto disp = make_shared<expr>();
						auto add_disp = make_shared<expr>();

						disp->value = operand->mem.disp.value;

						add_disp->operation.type = optr::add;
						add_disp->left = node;
						add_disp->right = disp;
						node = add_disp;
					}

					// if the mem pointer is being dereferenced and not just calculated
					if (operand->mem.type == ZYDIS_MEMOP_TYPE_MEM)
					{
						auto deref = make_shared<expr>();

						deref->operation.type = optr::deref;
						deref->left = node;
						node = deref;
					}

					break;
				case ZYDIS_OPERAND_TYPE_IMMEDIATE:
					node = make_shared<expr>();

					// does sign-age matter or is that redundant info?

					if (operand->imm.is_relative)
					{
						ZydisCalcAbsoluteAddress(&instr, operand, address, &node->value);
					}
					else
					{
						node->value = operand->imm.value.u;
					}

					break;
				case ZYDIS_OPERAND_TYPE_POINTER:

					// segment:offset
					//break;
				default:
					printf("Unhandled destination operand type\n");
				}

				if (operand->actions & ZYDIS_OPERAND_ACTION_MASK_WRITE)
				{
					operate->left = node;

					if (operand->type != ZYDIS_OPERAND_TYPE_REGISTER)
					{
						// can two operands have a write action?
						tail->type = itype::assign;
						tail->expr = operate;
					}
				}
				else if (operand->actions & ZYDIS_OPERAND_ACTION_MASK_READ
					|| operand->type == ZYDIS_OPERAND_TYPE_MEMORY)
				{
					operate->right = node;
				}
			}

			// update register
			if (updateReg) *updateReg = operate;
		}

		if (instr.meta.branch_type != ZYDIS_BRANCH_TYPE_NONE)
		{
			uint64_t branchAddress;
			shared_ptr<expr> branchRegs[ZYDIS_REGISTER_MAX_VALUE] = { nullptr };

			// use Ex version to handle register values
			ZydisCalcAbsoluteAddress(&instr, &instr.operands[0], address, &branchAddress);
			
			// skip if branch only known at runtime
			if (instr.meta.category != ZYDIS_CATEGORY_CALL
				&& !instr.operands[0].imm.is_relative)
				break;

			// find address of branch-to
			itr = instructions.find(branchAddress);

			if (itr != instructions.end())
			{
				// if jump handle local variables

				// use found instruction
				tail->branch = itr->second;
			}
			else
			{
				// if jump pass registers
				if (instr.meta.category != ZYDIS_CATEGORY_CALL)
				{
					for (int i = 0; i < ZYDIS_REGISTER_MAX_VALUE; i++)
					{
						branchRegs[i] = registers[i];
					}

					tail->type = itype::call;

					
				}
				else
				{
					tail->type = itype::call;
					tail->expr = make_shared<expr>();
					tail->expr->value = branchAddress;

					// figure out args

					auto newFunc = make_shared<func>();
					stringstream sTemp;
					sTemp << hex << branchAddress;
					newFunc->code = tail->branch;
					newFunc->name = "sub_";
					newFunc->name += string(sTemp.str());
					functions.insert({ branchAddress, newFunc });
				}

				instructions.insert({ address, tail });
				
				tail->branch = make_shared<icode>();
				genIRx64(tail->branch, branchRegs, branchAddress, nullptr);
			}

			
			if (instr.meta.category != ZYDIS_CATEGORY_CALL)
			{
				if (instr.meta.branch_type == ZYDIS_BRANCH_TYPE_FAR)
					printf("Long Jmp\n");
				else
					printf("Short Jmp\n");
			}
		}
		else
		{
			instructions.insert({ address, tail });
		}

		address += instr.length;
		codeSize -= instr.length;

		if (tail->type != itype::none)
		{
			tail->next = make_shared<icode>();
			tail = tail->next;
		}
	}
}

string evaluate(shared_ptr<expr> expr, shared_ptr<func> func)
{
	string temp;

	if (!expr->left) // leaf node
	{
		stringstream stream;

		switch (expr->type)
		{
		case idtype::none:
			stream << hex << expr->value;
			return string(stream.str());
		case idtype::var:
			return func->getVar(expr->value, varType::stack)->name;
		case idtype::reg:
			return STR_REGISTER[expr->id].data;
		default:
			printf("Unhandled idtype: %i\n", expr->type);
			return "";
		}
	}

	switch (expr->operation.type)
	{
	case optr::mov:
		break;
	case optr::add:
		temp = " + ";
		break;
	case optr::sub:
		temp = " - ";
		break;
	case optr::mul:
		temp = " * ";
		break;
	case optr::div:
		temp = " / ";
		break;
	case optr::or:
		temp = " | ";
		break;
	case optr::xor:
		temp = " ^ ";
		break;
	case optr::and:
		temp = " & ";
		break;
	case optr::not:
		return " ~" + evaluate(expr->left, func);
	case optr::shl:
		temp = " << ";
		break;
	case optr::shr:
		temp = " >> ";
		break;
	case optr::deref:
		return "*" + evaluate(expr->left, func);
	case optr::ref:
		return "&" + evaluate(expr->left, func);
	default:
		printf("Unhandled optr: %x\n", expr->operation.type);
		return "";
	}

	return evaluate(expr->left, func) + temp + evaluate(expr->right, func);
}

shared_ptr<expr> reduceTree(shared_ptr<expr> tree, shared_ptr<func> func)
{
	if (!tree->left) return tree;

	tree->left = reduceTree(tree->left, func);
	if (tree->right)
		tree->right = reduceTree(tree->right, func);

	// ignore past assignments
	if (!tree->left->left && tree->operation.type == optr::mov)
	{
		tree = tree->right;
	}

	// deref and ref cancel out
	if (tree->operation.type == optr::deref && tree->left->operation.type == optr::ref)
	{
		// need to adjust type too
		
		tree = tree->left->left;
	}

	// if operands are leaf nodes
	if (tree->left && !tree->left->left && (!tree->right || !tree->right->left))
	{
		auto newNode = make_shared<expr>();

		switch (tree->operation.type)
		{
		case optr::mov:
			return tree;
		case optr::add:
		{
			if (tree->left->type == idtype::reg)
			{
				if (tree->left->id == ZYDIS_REGISTER_RSP)
				{
					auto ref = make_shared<expr>();
					ref->left = newNode;
					ref->operation.type = optr::ref;

					// create variable if this is first reference
					func->getVar(tree->right->value, varType::stack);

					newNode->value = tree->right->value;
					newNode->type = idtype::var;
					return ref;
				}
			}

			newNode->value = tree->left->value + tree->right->value;
			return newNode;
		}
		case optr::sub:
			return tree;
		case optr::mul:
			return tree;
		case optr::div:
			return tree;
		case optr::or:
			return tree;
		case optr::xor:
			return tree;
		case optr::and:
			return tree;
		case optr::not:
			return tree;
		case optr::shl:
			return tree;
		case optr::shr:
			return tree;
		case optr::deref:
			return tree;
		case optr::ref:
			return tree;
		default:
			printf("Unhandled optr: %x\n", tree->operation.type);
			return tree;
		}
	}

	return tree;
}

string decompile(shared_ptr<func> func)
{
	string ccode = func->name + "()\n{\n";

	// print variables

	// cycle through icode
	for (auto ir = func->code; ir != nullptr; ir = ir->next)
	{
		// handle local variables
		// handle assignments
		// handle code constructs upon branches

		if (ir->expr) reduceTree(ir->expr, func);

		switch (ir->type)
		{
		case itype::assign:
			ccode += "\t" + evaluate(ir->expr->left, func);

			switch (ir->expr->operation.type)
			{
			case optr::mov:
				ccode += " = ";
				break;
			case optr::add:
				ccode += " += ";
				break;
			case optr::sub:
				ccode += " -= ";
				break;
			case optr::mul:
				ccode += " *= ";
				break;
			case optr::div:
				ccode += " /= ";
				break;
			case optr::or:
				ccode += " |= ";
				break;
			case optr::xor:
				ccode += " ^= ";
				break;
			case optr::and:
				ccode += " &= ";
				break;
			case optr::not:
				ccode += " ~= ";
				break;
			}

			if (ir->expr->operation.type != optr::not)
			{
				ccode += evaluate(ir->expr->right, func) + ";\n";
			}
			break;
		case itype::call:
		{
			auto itr = functions.find(ir->expr->value);
			if (itr != functions.end())
			{
				ccode += "\t";
				ccode += itr->second->name + "();\n";
			}
			break;
		}
		case itype::ret:
			ccode += "\t";
			ccode += "return;\n";
			break;
		}
	}

	ccode += "}\n";

	return ccode;
}

long readFileRaw(char* file, void** buf)
{
	long fsize;

	FILE* fd = fopen(file, "rb");
	if (!fd)
	{
		printf("[-] Unable to open file: %s\n", file);
		exit(1);
	}

	fseek(fd, 0, SEEK_END);
	fsize = ftell(fd);
	rewind(fd);

	*buf = malloc(fsize);
	if (!*buf) exit(1);

	fread(*buf, 1, fsize, fd);
	fclose(fd);

	return fsize;
}