#include "decompiler.hpp"

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

	// recursively generate IR (beginning with start)
	parse(ir, registers, baseAddress, nullptr);

	// decompile
	string src = decompile(result.first->second);

	printf("\n");
	printf(src.c_str());
}