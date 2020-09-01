#include "decompiler.hpp"

ZydisDecoder decoder;
ZydisFormatter formatter;
map<uint64_t, shared_ptr<func>> functions;
map<uint64_t, shared_ptr<icode>> instructions;
uint64_t baseAddress = 0x007FFFFFFF400000;
uint32_t codeSize = 0;
char instrStr[256];
void* bin;

shared_ptr<expr> getRegister(shared_ptr<expr> registers[ZYDIS_REGISTER_MAX_VALUE], uint64_t address, uint64_t value)
{
	if (value == ZYDIS_REGISTER_RIP)
	{
		auto addr = make_shared<expr>();

		addr->type = leaf::raw;
		addr->value = address;

		return addr;
	}

	auto temp = registers[value];
	if (!temp)
	{
		temp = make_shared<expr>();
		temp->type = leaf::reg;
		temp->value = value;
	}

	return temp;
}

void parse(shared_ptr<icode> tail, shared_ptr<expr> registers[ZYDIS_REGISTER_MAX_VALUE], uint64_t address, shared_ptr<var> args)
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

		// Format & print the instruction
		ZydisFormatterFormatInstruction(&formatter, &instr, instrStr, sizeof(instrStr), address);
		printf("%016" PRIX64 "  %s\n", address, instrStr);

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
		case ZYDIS_MNEMONIC_JNL:
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
		case ZYDIS_MNEMONIC_IMUL:
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
					node = getRegister(registers, address + instr.length, operand->reg.value);

					if (operand->actions & ZYDIS_OPERAND_ACTION_MASK_WRITE)
					{
						if (operand->reg.value == ZYDIS_REGISTER_RFLAGS)
						{
							node = nullptr;
						}
						else
						{
							updateReg = &registers[operand->reg.value];
						}
					}

					break;
				}
				case ZYDIS_OPERAND_TYPE_MEMORY:
					node = getRegister(registers, address + instr.length, operand->mem.base);

					// segment/index, register, scale?

					if (operand->mem.disp.has_displacement)
					{
						auto disp = make_shared<expr>();
						auto add_disp = make_shared<expr>();

						disp->type = leaf::raw;
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
					node->type = leaf::raw;

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

				if (node)
				{
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
					if (operand->actions & ZYDIS_OPERAND_ACTION_MASK_READ
						|| operand->type == ZYDIS_OPERAND_TYPE_MEMORY)
					{
						operate->right = node;
					}
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

					//tail->type = itype::call;


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
				parse(tail->branch, branchRegs, branchAddress, nullptr);
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