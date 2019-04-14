#include "linker.h"

int main(int argc, char* argv[])
{
	obj* objList = NULL;
	lib* libList = NULL;
	char* entrySymbol = "start";

	printf("[Mink] The MAGE linker - Written by Tserith\n\n");

	if ((unsigned int)argc < 2)
	{
		printf("Usage: mink [OBJECT FILES] [FLAGS]\n");
		printf("Flags:\n");
		printf("\t-l [LIBRARIES]\n");
		printf("\t-s [ENTRY SYMBOL] *Default: start*\n");
		exit(1);
	}

	int i = 1;
	
	// read in and parse object files to link
	for (; i < argc; i++)
	{
		if (argv[i][0] == '-')
		{
			i--;
			break;
		}

		readObj(&objList, argv[i]);
		printf("[+] Parsed %s", argv[i]);
	}

	for (i++; i < argc; i++)
	{
		if (!strcmp(argv[i], "-l"))
		{
			for (i++; i < argc; i++)
			{
				if (argv[i][0] == '-')
				{
					i--;
					break;
				}

				uint8_t len = (uint8_t)strnlen(argv[i], 32);
				lib* tempLib = (lib*)malloc(sizeof(lib));
				tempLib->name = (char*)malloc(len + 1);
				strncpy(tempLib->name, argv[i], len + 1);

				tempLib->next = libList;
				libList = tempLib;
			}
		}
		else if (!strcmp(argv[i], "-s"))
		{
			i++;
			if (i < argc)
			{
				if (argv[i][0] == '-')
				{
					i--;
					break;
				}
				entrySymbol = argv[i];
			}
		}
	}

	if(!objList)
	{
		printf("[-] No objects provided to link\n");
		exit(1);
	}

	// link object files into MAGE executable format
	link(objList);

	return 0;
}

void readObj(obj** objList, char* file)
{
	uint8_t* buf;
	long fsize;

	FILE* fd = fopen(file, "rb");
	if (!fd)
	{
		printf("[-] Unable to open provided file: %s\n", file);
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
	
	tempObj->next = *objList;
	*objList = tempObj;

	free(buf);
}

obj* parseCOFF(uint8_t* buf, long size) // should probably use size to prevent segfaults
{
	PIMAGE_FILE_HEADER header = buf;
	PIMAGE_SYMBOL symTable = buf + header->PointerToSymbolTable;
	uint8_t* strTable = (uint8_t*)symTable + (sizeof(IMAGE_SYMBOL) * header->NumberOfSymbols) + 4;
	uint16_t sectionCount = header->NumberOfSections;
	obj* tempObj = NULL;

	// find .text section
	for (int i = 0; i < sectionCount; i++)
	{
		PIMAGE_SECTION_HEADER section = buf + sizeof(IMAGE_FILE_HEADER) + (sizeof(IMAGE_SECTION_HEADER) * i);

		if (!strncmp(section->Name, ".text", 8))
		{
			// make new object
			tempObj = (obj*)malloc(sizeof(obj));

			tempObj->csize = section->SizeOfRawData;
			tempObj->code = malloc(tempObj->csize);
			memcpy(tempObj->code, buf + section->PointerToRawData, tempObj->csize);

			if (header->NumberOfSymbols)
			{
				for (uint16_t j = 0; j < header->NumberOfSymbols; j++)
				{
					PIMAGE_SYMBOL symbol = (uint8_t*)symTable + (sizeof(IMAGE_SYMBOL) * j);

					if (symbol->SectionNumber == i + 1)
					{
						// make new symbol
						sym* tempSym = (sym*)malloc(sizeof(sym));
						size_t len;

						if (symbol->N.Name.Short) // short name
						{
							len = strnlen(symbol->N.ShortName, 8);
							tempSym->name = (char*)malloc(len + 1);
							strncpy(tempSym->name, symbol->N.ShortName, len + 1);
							tempSym->name[len] = '\0';
						}
						else // long name (stored in string table)
						{
							len = strnlen(strTable + symbol->N.Name.Long - 4, 64);
							tempSym->name = (char*)malloc(len + 1);
							strncpy(tempSym->name, strTable + symbol->N.Name.Long - 4, len + 1);
							tempSym->name[len] = '\0';
						}
					}
				}
			}
			else tempObj->symbols = NULL;
			
			if (section->NumberOfRelocations)
			{
				for (int j = 0; j < section->NumberOfRelocations; j++)
				{
					PIMAGE_RELOCATION reloc = (uint8_t*)buf + section->PointerToRelocations + (sizeof(IMAGE_RELOCATION) * j);
					PIMAGE_SYMBOL symbol = (uint8_t*)symTable + (sizeof(IMAGE_SYMBOL) * reloc->SymbolTableIndex);

					// make new relocation
					rel* tempRel = (rel*)malloc(sizeof(rel));
					tempRel->addr = reloc->VirtualAddress;
					tempRel->value = symbol->Value;

					size_t len;

					if (symbol->N.Name.Short) // short name
					{
						len = strnlen(symbol->N.ShortName, 8);
						tempRel->name = (char*)malloc(len + 1);
						strncpy(tempRel->name, symbol->N.ShortName, len + 1);
						tempRel->name[len] = '\0';
					}
					else // long name (stored in string table)
					{
						len = strnlen(strTable + symbol->N.Name.Long - 4, 64);
						tempRel->name = (char*)malloc(len + 1);
						strncpy(tempRel->name, strTable + symbol->N.Name.Long - 4, len + 1);
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
	// combine .text sections
	// resolve relocations
	// add import table
}