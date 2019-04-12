#include "linker.h"

int main(int argc, char* argv[])
{
	obj* objList = NULL;
	char* entrySymbol = "start"; // change with argument

	if ((unsigned int)argc < 2)
	{
		printf("[-] No objects provided to link\n");
		exit(1);
	}

	// read in object files to link
	for (int i = 1; i < argc - 1; i++)
	{
		uint8_t* buf;
		long fsize;

		FILE* fd = fopen(argv[i], "rb");
		if (!fd)
		{
			printf("[-] Unable to open provided file: %s\n", argv[i]);
			exit(1);
		}

		fseek(fd, 0, SEEK_END);
		fsize = ftell(fd);
		rewind(fd);

		buf = (uint8_t*)malloc(fsize);
		fread(buf, 1, fsize, fd);
		fclose(fd);

		// parse object file
		obj* tempObj = parse(buf, fsize);

		if (objList) tempObj->next = objList;
		else tempObj->next = NULL;
		objList = tempObj;

		free(buf);
	}

	// link object files into MAGE executable format
	link(objList);

	return 0;
}

obj* parseCOFF(uint8_t* buf, long size) // should probably use size to prevent segfaults
{
	PIMAGE_FILE_HEADER header = (PIMAGE_FILE_HEADER)buf;
	PIMAGE_SYMBOL symTable = (PIMAGE_SYMBOL)buf + header->PointerToSymbolTable;
	uint8_t* strTable = buf + (sizeof(IMAGE_SYMBOL) * header->NumberOfSymbols);
	uint16_t sectionCount = header->NumberOfSections;
	obj* tempObj = NULL;

	// find .text section
	for (int i = 0; i < sectionCount - 1; i++)
	{
		PIMAGE_SECTION_HEADER section = (PIMAGE_SECTION_HEADER)buf
										+ sizeof(IMAGE_FILE_HEADER)
										+ (sizeof(IMAGE_SECTION_HEADER) * i);

		if (!strncmp((char*)section, ".text", 8))
		{
			// make new object
			tempObj = (obj*)malloc(sizeof(obj));

			tempObj->csize = section->SizeOfRawData;
			tempObj->code = malloc(tempObj->csize);
			memcpy(tempObj->code, buf + section->PointerToRawData, tempObj->csize);

			if (header->NumberOfSymbols)
			{
				for (uint16_t j = 0; j < header->NumberOfSymbols - 1; j++)
				{
					PIMAGE_SYMBOL symbol = symTable + (sizeof(IMAGE_SYMBOL) * j);

					if (symbol->SectionNumber == i + 1)
					{
						// make new symbol
						sym* tempSym = (sym*)malloc(sizeof(sym));
						size_t len;

						if (symbol->N.Name.Short) // short name
						{
							len = strnlen(symbol->N.ShortName, 8);
							tempSym->name = (char*)malloc(len + 1);
							strncpy(tempSym->name, symbol->N.ShortName, 8);
							tempSym->name[len] = '\0';
						}
						else // long name (stored in string table)
						{
							len = strnlen(strTable + symbol->N.Name.Long - 4, 64);
							tempSym->name = (char*)malloc(len + 1);
							strncpy(tempSym->name, strTable + symbol->N.Name.Long - 4, 64);
							tempSym->name[len] = '\0';
						}
					}
				}
			}
			else tempObj->symbols = NULL;

			if (section->NumberOfRelocations)
			{
				for (int j = 0; j < section->NumberOfRelocations - 1; j++)
				{
					PIMAGE_RELOCATION reloc = (PIMAGE_RELOCATION)buf
												+ section->PointerToRelocations
												+ (sizeof(IMAGE_RELOCATION) * j);
					PIMAGE_SYMBOL symbol = symTable
											+ (sizeof(IMAGE_SYMBOL)
												* reloc->SymbolTableIndex);

					// make new relocation
					rel* tempRel = (rel*)malloc(sizeof(rel));
					tempRel->addr = reloc->VirtualAddress;
					tempRel->value = symbol->Value;

					size_t len;

					if (symbol->N.Name.Short) // short name
					{
						len = strnlen(symbol->N.ShortName, 8);
						tempRel->name = (char*)malloc(len + 1);
						strncpy(tempRel->name, symbol->N.ShortName, 8);
						tempRel->name[len] = '\0';
					}
					else // long name (stored in string table)
					{
						len = strnlen(strTable + symbol->N.Name.Long - 4, 64);
						tempRel->name = (char*)malloc(len + 1);
						strncpy(tempRel->name, strTable + symbol->N.Name.Long - 4, 64);
						tempRel->name[len] = '\0';
					}
				}
			}
			else tempObj->relocs = NULL;
		}
	}

	return tempObj;
}

obj* parseELF(uint8_t* buf, long size)
{
	obj* tempObj = NULL;

	return tempObj;
}

void link(obj* objList)
{

}