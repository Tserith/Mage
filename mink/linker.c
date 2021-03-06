#include "linker.h"

int main(int argc, char* argv[])
{
	obj* objList = NULL;
	lib* libList = NULL;
	char* entrySymbol = DEFAULT_ENTRY_SYMBOL;
	char* outFile = DEFAULT_OUT_FILE;

	printf("[Mink] The MAGE linker - Written by Tserith\n\n");

	if ((unsigned int)argc < 2)
	{
		printf("Usage: mink [OBJECT FILES] [FLAGS]\n");
		printf("Flags:\n");
		printf("\t-l [LIBRARIES]\n");
		printf("\t-s [ENTRY SYMBOL] *Default: main\n");
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
		printf("[+] Parsed %s\n", argv[i]);
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

				uint16_t len = (uint16_t)strnlen(argv[i], MAX_SYM_LEN);
				lib* tempLib = (lib*)malloc(sizeof(lib));
				tempLib->name = argv[i];

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
		else if (!strcmp(argv[i], "-o"))
		{
			i++;
			if (i < argc)
			{
				if (argv[i][0] == '-')
				{
					i--;
					break;
				}
				outFile = argv[i];
			}
		}
	}

	if(!objList)
	{
		printf("[-] No objects provided to link\n");
		exit(1);
	}

	// link object files into MAGE executable format
	void* bin = NULL;
	uint16_t binSize = link(&bin, objList, libList, entrySymbol);

	printf("[+] Linked object files into MAGE binary\n");
	printf("\tSize:\t\t0x%x bytes\n", binSize);
	printf("\tEntry address:\t0x%x\n", ((MAGE_HEADER*)bin)->entry);

	// write out MAGE executable
	FILE* fd = fopen(outFile, "wb");
	if (!fd)
	{
		printf("[-] Unable to write to file: %s\n", outFile);
		exit(1);
	}

	fwrite(bin, 1, binSize, fd);

	printf("[+] Wrote %s binary to disk\n", outFile);

	free(bin);

	for (obj* obj = objList; obj != NULL; objList = obj)
	{
		for (sym* sym = obj->symbols; sym != NULL; obj->symbols = sym)
		{
			sym = sym->next;
			free(obj->symbols);
		}

		obj = obj->next;
		free(objList->code);
		free(objList);
	}

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
	PIMAGE_FILE_HEADER header = (void*)buf;
	PIMAGE_SYMBOL symTable = (void*)(buf + header->PointerToSymbolTable);
	uint8_t* strTable = (uint8_t*)symTable + (sizeof(IMAGE_SYMBOL) * header->NumberOfSymbols) + 4;
	uint16_t sectionCount = header->NumberOfSections;

	// make new object
	obj* tempObj = (obj*)malloc(sizeof(obj));
	tempObj->symbols = NULL;
	sym* tempSym = NULL;

	// find section that starts with .text
	for (int i = 0; i < sectionCount; i++)
	{
		PIMAGE_SECTION_HEADER section = (void*)(buf + sizeof(IMAGE_FILE_HEADER) + (sizeof(IMAGE_SECTION_HEADER) * i));

		if (!strncmp(section->Name, ".text", 5))
		{
			tempObj->csize = section->SizeOfRawData;
			tempObj->code = (uint8_t*)malloc(tempObj->csize);
			memcpy(tempObj->code, buf + section->PointerToRawData, tempObj->csize);
			
			// parse symbols
			for (uint16_t j = 0; j < header->NumberOfSymbols; j++)
			{
				// auxiliary symbols should have empty names to avoid collisions!
				PIMAGE_SYMBOL symbol = (void*)((uint8_t*)symTable + (sizeof(IMAGE_SYMBOL) * j));
				
				// make new symbol
				if (tempObj->symbols != NULL)
				{
					tempSym->next = (sym*)malloc(sizeof(sym));
					tempSym = tempSym->next;
				}
				else
				{
					tempObj->symbols = (sym*)malloc(sizeof(sym));
					tempSym = tempObj->symbols;
				}

				tempSym->next = NULL;
				tempSym->value = symbol->Value;
				tempSym->class = symbol->StorageClass;
				tempSym->relocs = NULL;
				uint16_t len;

				if (symbol->N.Name.Short) // short name
				{
					len = (uint16_t)strnlen(symbol->N.ShortName, 8);
					tempSym->name = (char*)malloc(len + 1);
					strncpy(tempSym->name, symbol->N.ShortName, len + 1);
					tempSym->name[len] = '\0';
				}
				else // long name (stored in string table)
				{
					len = (uint16_t)strnlen(strTable + symbol->N.Name.Long - 4, MAX_SYM_LEN);
					tempSym->name = (char*)malloc(len + 1);
					strncpy(tempSym->name, strTable + symbol->N.Name.Long - 4, len + 1);
					tempSym->name[len] = '\0';
				}
			}

			if (!header->NumberOfSymbols) tempObj->symbols = NULL;
			
			// parse relocations
			if (section->NumberOfRelocations)
			{
				for (int j = 0; j < section->NumberOfRelocations; j++)
				{
					PIMAGE_RELOCATION reloc = (void*)(buf + section->PointerToRelocations + (sizeof(IMAGE_RELOCATION) * j));
					tempSym = tempObj->symbols;

					if (reloc->Type == IMAGE_REL_AMD64_REL32)
					{
						// set symbol as relocation
						for (int k = 0; k < (int)reloc->SymbolTableIndex; k++) tempSym = tempSym->next;

						rel* tempRel = (rel*)malloc(sizeof(rel));

						tempRel->vaddr = reloc->VirtualAddress;
						tempRel->next = tempSym->relocs;
						tempSym->relocs = tempRel;
					}
				}
			}
		}
	}

	return tempObj;
}

obj* parseELF(uint8_t* buf, long size)
{
	obj* tempObj = NULL;

	return tempObj;
}

uint16_t link(void** bin, obj* objList, lib* libList, char* entrySym)
{
	uint8_t* buf = (uint8_t*)malloc(sizeof(MAGE_HEADER));
	uint16_t rptr, cptr, buflen = sizeof(MAGE_HEADER);
	uint32_t entry = -1;

	// calculate pointer to relocations
	for (lib* lib = libList; lib != NULL; lib = lib->next) buflen += sizeof(MAGE_IMPORT_ENTRY);
	rptr = buflen;

	// resolve relocations between objects
	for (obj* obj1 = objList; obj1 != NULL; obj1 = obj1->next)
	{
		for (sym* sym1 = obj1->symbols; sym1 != NULL; sym1 = sym1->next)
		{
			uint32_t offset = 0;

			// search for symbol in other objects
			for (obj* obj2 = objList; sym1->relocs && obj2 != NULL; obj2 = obj2->next)
			{
				if (obj1 != obj2)
				{
					for (sym* sym2 = obj2->symbols; sym2 != NULL; sym2 = sym2->next)
					{
						if (sym2->class == IMAGE_SYM_CLASS_EXTERNAL
							&& !strncmp(sym1->name, sym2->name, MAX_SYM_LEN))
						{
							rel* reloc = sym1->relocs;

							while (reloc != NULL)
							{
								*(MAGE_RELOC_VADDR*)(obj1->code + reloc->vaddr) = offset + sym2->value;

								sym1->relocs = NULL;
								rel* tmp = reloc;
								reloc = reloc->next;
								free(tmp);
							}
							
							break;
						}
					}
				}

				offset += obj2->csize;
			}
		}
	}

	for (obj* obj = objList; obj != NULL; obj = obj->next)
	{
		for (sym* sym = obj->symbols; sym != NULL; sym = sym->next)
		{
			for (rel* rel = sym->relocs; rel != NULL; rel = rel->next) buflen += sizeof(MAGE_RELOC_ENTRY);

			// search for entry symbol
			if (sym->class == IMAGE_SYM_CLASS_EXTERNAL && !strncmp(sym->name, entrySym, MAX_SYM_LEN))
				entry = sym->value;
		}
	}

	if (entry == -1)
	{
		printf("[-] Entry symbol could not be found: %s\n", entrySym);
		exit(1);
	}

	// allocate memory for import and relocation table
	buf = realloc(buf, buflen);
	
	uint16_t addr = sizeof(MAGE_HEADER);

	// create string table and fill in addresses in import and relocation tables
	for (lib* lib = libList; lib != NULL; lib = lib->next)
	{
		uint16_t len = (uint16_t)strnlen(lib->name, MAX_SYM_LEN);

		// remove .lib
		if (!strncmp(lib->name + len - 4, ".lib", 4)) len -= 4;

		buf = realloc(buf, buflen + len + 1);
		strncpy(buf + buflen, lib->name, len);
		buf[buflen + len] = '\0';

		buflen += len + 1;

		*(MAGE_STR_PTR*)(buf + addr) = buflen - len - 1;
		addr += sizeof(MAGE_IMPORT_ENTRY);
	}
	for (obj* obj = objList; obj != NULL; obj = obj->next)
	{
		for (sym* sym = obj->symbols; sym != NULL; sym = sym->next)
		{
			uint16_t len = 0;

			if (sym->relocs)
			{
				len = (uint16_t)strnlen(sym->name, MAX_SYM_LEN);
				buf = realloc(buf, buflen + len + 1);
				strncpy(buf + buflen, sym->name, len);
				buf[buflen + len] = '\0';
				buflen += len + 1;
			}
			for (rel* rel = sym->relocs; rel != NULL; rel = rel->next)
			{
				*(MAGE_STR_PTR*)(buf + addr) = buflen - len - 1;
				addr += sizeof(MAGE_RELOC_ENTRY);
			}
		}
	}
	
	cptr = buflen;

	// add code sections
	for (obj* obj = objList; obj != NULL; obj = obj->next)
	{
		buf = realloc(buf, buflen + obj->csize);
		memcpy(buf + buflen, obj->code, obj->csize);
		buflen += obj->csize;
	}

	uint32_t offset = 0;
	uint16_t relocs = rptr;

	// create thunks for relocations and fill in relocation table
	for (obj* obj = objList; obj != NULL; obj = obj->next)
	{
		for (sym* sym = obj->symbols; sym != NULL; sym = sym->next)
		{
			if (sym->relocs)
			{
				buflen += 14;
				buf = realloc(buf, buflen);

				*(uint64_t*)(buf + buflen - 14) = 0x25FF; // jmp far (6 bytes)
				*(uint64_t*)(buf + buflen - 8) = 0; // absolute address (dlinked)
			}
			for (rel* rel = sym->relocs; rel != NULL; rel = rel->next)
			{
				// point call operands to thunk
				*(uint32_t*)(buf + cptr + offset + rel->vaddr) = buflen - cptr - offset - rel->vaddr - 18;
				((MAGE_RELOC_ENTRY*)(buf + relocs))->vaddr = (MAGE_RELOC_VADDR)buflen - cptr - 8;
				relocs += sizeof(MAGE_RELOC_ENTRY);
			}
		}

		offset += obj->csize;
	}

	MAGE_HEADER* header = (void*)buf;

	header->magic = MAGE_MAGIC;
	header->entry = entry;
	header->csize = buflen - cptr;
	header->rptr = rptr;
	header->cptr = cptr;
	
	*bin = buf;

	return buflen;
}