#include "decompiler.hpp"

map<uint64_t, shared_ptr<func>> functions;
map<uint64_t, shared_ptr<icode>> instructions;

ZydisDecoder decoder;
ZydisFormatter formatter;

uint64_t runtimeAddress = 0x007FFFFFFF400000;
char instrStr[256];
void* bin;
uint32_t codeSize = 0;

int main(int argc, char* argv[])
{
	shared_ptr<expr> registers[REGISTER_COUNT] = { nullptr };
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
	functions.insert({ runtimeAddress + startAddress, newFunc });

	// generate IR for each function (beginning with start)
	genIRx64(ir, registers, runtimeAddress, nullptr);

	// decompile
}

void genIRx64(shared_ptr<icode> tail, shared_ptr<expr> registers[REGISTER_COUNT], uint64_t address, shared_ptr<var> args)
{
	ZydisDecodedInstruction instr;

	if (args)
	{
		// handle args
	}

	// iterate through instructions until ret
	while (ZydisDecoderDecodeBuffer(&decoder, &((uint8_t*)bin)[address - runtimeAddress], codeSize, &instr))
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


		if (instr.operands[0].actions & ZYDIS_OPERAND_ACTION_MASK_WRITE)
		{
			//printf("Write\n");
		}

		if (instr.meta.category == ZYDIS_CATEGORY_RET)
		{
			printf("Ret\n");
			tail->type = ICODE_RET;
			break;
		}

		if (instr.meta.branch_type != ZYDIS_BRANCH_TYPE_NONE)
		{
			uint64_t branchAddress;
			shared_ptr<expr> branchRegs[REGISTER_COUNT] = { nullptr };

			// use Ex version to handle register values
			ZydisCalcAbsoluteAddress(&instr, &instr.operands[0], address, &branchAddress);
			
			// exit if branch only known at runtime
			if (instr.meta.category != ZYDIS_CATEGORY_RET
				&& instr.meta.category != ZYDIS_CATEGORY_CALL
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
					for (int i = 0; i < REGISTER_COUNT; i++)
					{
						branchRegs[i] = registers[i];
					}

					tail->type = ICODE_CALL;
				}
				else
				{
					// figure out args

					auto newFunc = make_shared<func>();
					newFunc->code = tail->branch;
					functions.insert({ branchAddress, newFunc });
				}

				instructions.insert({ address, tail });
				
				tail->branch = make_shared<icode>();
				genIRx64(tail->branch, branchRegs, branchAddress, nullptr);
			}

			
			if (instr.meta.category != ZYDIS_CATEGORY_CALL)
			{
				if (instr.meta.branch_type == ZYDIS_BRANCH_TYPE_FAR) printf("Long Jmp\n");
				else printf("Short Jmp\n");
			}
		}
		else
		{
			instructions.insert({ address, tail });
		}

		address += instr.length;
		codeSize -= instr.length;
		tail->next = make_shared<icode>();
		tail = tail->next;
	}
}

string decompile(shared_ptr<icode> IR)
{
	string ccode;

	// cycle through icode
	// handle local variables
	// handle assignments
	// handle code constructs upon branches

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