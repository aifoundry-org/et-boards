/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This file is intended to be included ONLY from the esperanto_load_elf.c file
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef FUNCTION_NAME
#error "FUNCTION_NAME not defined!"
#endif
#ifndef TYPE_NAME
#error "TYPE_NAME not defined!"
#endif
#ifndef VAR_NAME
#error "VAR_NAME not defined!"
#endif

#ifdef ELF_HEADER_TYPE
#undef ELF_HEADER_TYPE
#endif
#define ELF_HEADER_TYPE TYPE_NAME(Ehdr)

#ifdef ELF_PH_TABLE_TYPE
#undef ELF_PH_TABLE_TYPE
#endif
#define ELF_PH_TABLE_TYPE TYPE_NAME(Phdr)

#ifdef ELF_SH_TABLE_TYPE
#undef ELF_SH_TABLE_TYPE
#endif
#define ELF_SH_TABLE_TYPE TYPE_NAME(Shdr)

#ifdef ELF_SM_TABLE_TYPE
#undef ELF_SM_TABLE_TYPE
#endif
#define ELF_SM_TABLE_TYPE TYPE_NAME(Sym)

#ifdef FIND_SHSTR_TABLE_FN
#undef FIND_SHSTR_TABLE_FN
#endif
#define FIND_SHSTR_TABLE_FN FUNCTION_NAME(find, shstr_table)

#ifdef FIND_STR_TABLE_FN
#undef FIND_STR_TABLE_FN
#endif
#define FIND_STR_TABLE_FN FUNCTION_NAME(find, str_table)

#ifdef FIND_IMAGE_VERSION_INFO_FN
#undef FIND_IMAGE_VERSION_INFO_FN
#endif
#define FIND_IMAGE_VERSION_INFO_FN FUNCTION_NAME(find, image_info)

#ifdef PH_TABLE_PARSE_FN
#undef PH_TABLE_PARSE_FN
#endif
#define PH_TABLE_PARSE_FN FUNCTION_NAME(parse, program_header_table)

#ifdef HEADER_PARSE_FN
#undef HEADER_PARSE_FN
#endif
#define HEADER_PARSE_FN FUNCTION_NAME(parse, header)

static int FIND_SHSTR_TABLE_FN(
    const ELF_SH_TABLE_TYPE * section_header_table,
    uint32_t section_header_table_size, 
    const uint8_t * image, 
    uint32_t image_length, 
    const char ** shstr_table,
    uint32_t * shstr_table_size
) {
    uint32_t n;
    const char * string_table = NULL;
    uint32_t string_table_size, string_table_end_offset;
    const char * section_name;

    for (n = 0; n < section_header_table_size; n++) {
        if (SHT_STRTAB == section_header_table[n].sh_type) {
            string_table_end_offset = (uint32_t)(section_header_table[n].sh_offset + section_header_table[n].sh_size);
            if (section_header_table[n].sh_offset < sizeof(ELF_HEADER_TYPE) || image_length < string_table_end_offset) {
                fprintf(stderr, "Error in %s: Invalid string table offset or size!\n", __func__);
                return -1;
            }
            string_table = (const char *)(image + section_header_table[n].sh_offset);
            string_table_size = (uint32_t)section_header_table[n].sh_size;
            if (0 != string_table[0]) {
                fprintf(stderr, "Error in %s: First byte of the string table is not 0!\n", __func__);
                return -1;
            }
            if (0 != string_table[string_table_size-1]) {
                fprintf(stderr, "Error in %s: Last byte of the string table is not 0!\n", __func__);
                return -1;
            }

            // check if the section name is .shstrtab
            if (section_header_table[n].sh_name >= string_table_size) {
                continue;
            }
            section_name = string_table + section_header_table[n].sh_name;
            if (0 == strcmp(section_name, ".shstrtab")) {
                *shstr_table = string_table;
                *shstr_table_size = string_table_size;
                return 0;
            }
        }
    }

    return -1;    
}

static int FIND_STR_TABLE_FN(
    const ELF_SH_TABLE_TYPE * section_header_table,
    uint32_t section_header_table_size, 
    const uint8_t * image, 
    uint32_t image_length, 
    const char ** str_table,
    uint32_t * str_table_size
) {
    uint32_t n;
    const char * shstr_table = NULL;
    uint32_t shstr_table_size;
    const char * string_table = NULL;
    uint32_t string_table_size, string_table_end_offset;
    const char * section_name;

    if (0 != FIND_SHSTR_TABLE_FN(section_header_table, section_header_table_size, image, image_length, &shstr_table, &shstr_table_size)) {
        fprintf(stderr, "Warning in %s: string table section .shstrtab not found!\n", __func__);
    }

    for (n = 0; n < section_header_table_size; n++) {
        if (SHT_STRTAB == section_header_table[n].sh_type) {
            string_table_end_offset = (uint32_t)(section_header_table[n].sh_offset + section_header_table[n].sh_size);
            if (section_header_table[n].sh_offset < sizeof(ELF_HEADER_TYPE) || image_length < string_table_end_offset) {
                fprintf(stderr, "Error in %s: Invalid string table offset or size!\n", __func__);
                return -1;
            }
            string_table = (const char *)(image + section_header_table[n].sh_offset);
            string_table_size = (uint32_t)section_header_table[n].sh_size;
            if (0 != string_table[0]) {
                fprintf(stderr, "Error in %s: First byte of the string table is not 0!\n", __func__);
                return -1;
            }
            if (0 != string_table[string_table_size-1]) {
                fprintf(stderr, "Error in %s: Last byte of the string table is not 0!\n", __func__);
                return -1;
            }

            if (NULL != shstr_table) {
                // check if the section name is .strtab
                if (section_header_table[n].sh_name >= shstr_table_size) {
                    continue;
                }
                section_name = shstr_table + section_header_table[n].sh_name;
                if (0 == strcmp(section_name, ".strtab")) {
                    *str_table = string_table;
                    *str_table_size = string_table_size;
                    return 0;
                }
            } else {
                fprintf(stderr, "Warning in %s: Using the section %u as the string table!\n", __func__, n);
                *str_table = string_table;
                *str_table_size = string_table_size;
                return 0;
            }
        }
    }

    return -1;    
}

static int FIND_IMAGE_VERSION_INFO_FN(
    const ELF_SH_TABLE_TYPE * section_header_table,
    uint32_t section_header_table_size, 
    const uint8_t * image, 
    uint32_t image_length, 
    uint32_t * image_info_addr_lo,
    uint32_t * image_info_addr_hi,
    uint32_t * image_info_symbol_size
) {
    uint32_t n;
    const ELF_SM_TABLE_TYPE * symbol_table = NULL;
    uint32_t symbol_table_size, symbol_table_end_offset;
    uint32_t symbol_entries_count;
    const char * string_table = NULL;
    uint32_t string_table_size;
    U_64_32_t symbol_address;

    *image_info_addr_lo = 0;
    *image_info_addr_hi = 0;

    if (0 != FIND_STR_TABLE_FN(section_header_table, section_header_table_size, image, image_length, &string_table, &string_table_size)) {
        fprintf(stderr, "Error in %s: String table section not found!\n", __func__);
        return -1;
    }

    // find symbol table
    for (n = 0; n < section_header_table_size; n++) {
        if (SHT_SYMTAB != section_header_table[n].sh_type) {
            continue;
        }

        if (NULL != symbol_table) {
            fprintf(stderr, "Error in %s: found more than one section of type SHT_SYMTAB!\n", __func__);
            return -1;
        }
        if (sizeof(ELF_SM_TABLE_TYPE) != section_header_table[n].sh_entsize) {
            fprintf(stderr, "Error in %s: size of the SHT_SYMTAB entry size is not %lu!\n", __func__, sizeof(ELF_SM_TABLE_TYPE));
            return -1;
        }
        symbol_table_end_offset = (uint32_t)(section_header_table[n].sh_offset + section_header_table[n].sh_size);
        if (section_header_table[n].sh_offset < sizeof(ELF_HEADER_TYPE) || image_length < symbol_table_end_offset) {
            fprintf(stderr, "Error in %s: Invalid symbol table offset or size!\n", __func__);
            return -1;
        }
        symbol_table = (const ELF_SM_TABLE_TYPE*)(image + section_header_table[n].sh_offset);
        symbol_table_size = (uint32_t)section_header_table[n].sh_size;
        symbol_entries_count = symbol_table_size / sizeof(ELF_SM_TABLE_TYPE);
        break;
    }

    if (NULL == symbol_table) {
        fprintf(stderr, "Error in %s: Symbol table section not found!\n", __func__);
        return -1;
    }

    // scan the symbol table for the g_imag_info symbol
    for (n = 0; n < symbol_entries_count; n++) {
        //if (STB_GLOBAL != ELF_ST_BIND(symbol_table[n].st_info)) {
        //    continue;
        //}
        if (STT_OBJECT != ELF_ST_TYPE(symbol_table[n].st_info)) {
            continue;
        }
        switch (symbol_table[n].st_shndx) {
        case SHN_UNDEF:
        case SHN_COMMON:
            continue;
        case SHN_ABS:
        default:
        	break;
        }
        if (symbol_table[n].st_name >= string_table_size) {
            fprintf(stderr, "Error in %s: Symbol %u name is outside the string table!\n", __func__, n);
            return -1;
        }
        if (0 != strcmp(string_table + symbol_table[n].st_name, IMAGE_VERSION_INFO_SYMBOL_NAME)) {
            continue;
        }

        symbol_address.u64 = symbol_table[n].st_value;
        *image_info_addr_lo = symbol_address.u32.lo;
        *image_info_addr_hi = symbol_address.u32.hi;
        *image_info_symbol_size = (uint32_t)symbol_table[n].st_size;
        return 0;
    }

    return 0;
}

static int PH_TABLE_PARSE_FN(
    const ELF_PH_TABLE_TYPE * program_header_table,
    uint32_t program_header_table_size, 
    const uint8_t * image, 
    uint32_t image_length, 
    uint8_t ** esperanto_image_code_and_data, 
    uint32_t * esperanto_image_code_and_data_length, 
    ESPERANTO_IMAGE_FILE_HEADER_t * esperanto_image_header
) {
    int rval;
    uint32_t total_load_size;
    uint32_t n;
    U_64_32_t address, load_size, memory_size;
    uint8_t * load_data = NULL;
    uint32_t offset, increment;
    uint32_t regions_count = 0;
    uint32_t load_size_adjustement = 0;

    // We will scan the ELF program header table twice:
    // - first time we will collect all the information (load address, memory address, load size and memory size), but we will not copy the code/data yet
    // - second time will will use previously collected information and we will copy the data from the original ELF file to the signed image.  We will also update the load offset.

    // scan the ELF progam header table for the first time
    total_load_size = 0;
    for (n = 0; n < program_header_table_size; n++) {
        switch (program_header_table[n].p_type) {
        case PT_NULL:
            fprintf(stderr, "Error in %s: region %u type (PT_NULL) is not supported (yet)!\n", __func__, n);
            return -1;
        
        case PT_LOAD:
            break;

	case PT_GNU_STACK:
	    break;

        case PT_DYNAMIC:
            fprintf(stderr, "Error in %s: region %u type (PT_DYNAMIC) is not supported!\n", __func__, n);
            return -1;

        case PT_INTERP:
            fprintf(stderr, "Error in %s: region %u type (PT_INTERP) is not supported!\n", __func__, n);
            return -1;

        case PT_NOTE:
            fprintf(stderr, "Error in %s: region %u type (PT_NOTE) is not supported (yet)!\n", __func__, n);
            return -1;

        case PT_PHDR:
            fprintf(stderr, "Error in %s: region %u type (PT_PHDR) is not supported (yet)!\n", __func__, n);
            return -1;
        
        default:
            fprintf(stderr, "Error in %s: region %u type (0x%x) is not supported (yet)!\n", __func__, n, program_header_table[n].p_type);
            return -1;
        }

        if (program_header_table[n].p_type == PT_GNU_STACK) {
	    fprintf(stderr, "Ignoring PT_GNU_STACK section.\n");
	    continue;
	}

        if (regions_count >= MAX_EXECUTABLE_IMAGE_LOAD_REGIONS_COUNT) {
            fprintf(stderr, "Error in %s: the number of regions has exceeded the maximum supported value %u!\n", __func__, MAX_EXECUTABLE_IMAGE_LOAD_REGIONS_COUNT);
            return -1;
        }
        
        address.u64 = program_header_table[n].p_paddr;
        if (0 != (address.u32.lo & 0x7F)) {
            fprintf(stderr, "Error in %s: region %u is not 128 bytes aligned!\n", __func__, n);
            return -1;
        }
        esperanto_image_header->info.image_info_and_signaure.info.secret_info.load_regions[regions_count].load_address_lo = address.u32.lo;
        esperanto_image_header->info.image_info_and_signaure.info.secret_info.load_regions[regions_count].load_address_hi = address.u32.hi;
        load_size.u64 = program_header_table[n].p_filesz;
        if (load_size.u32.hi > 0) {
            fprintf(stderr, "Error in %s: region %u load size is larger than 4 GB!\n", __func__, n);
            return -1;
        }
        if (0 != (load_size.u32.lo & 0x7F)) {
            // this region file size is not a multiple of 128 bytes!
            printf("%s: region %u load size is not a multiple of 128 bytes!\n", __func__, n);
            // we will have to pad it to the next multiple of 128 bytes
            load_size_adjustement = load_size_adjustement + 128 - (load_size.u32.lo & 0x7F);
        }
        esperanto_image_header->info.image_info_and_signaure.info.secret_info.load_regions[regions_count].load_size = load_size.u32.lo;
        memory_size.u64 = program_header_table[n].p_memsz;
        if (memory_size.u32.hi > 0) {
            fprintf(stderr, "Error in %s: region %u memory size is larger than 4 GB!\n", __func__, n);
            return -1;
        }
        esperanto_image_header->info.image_info_and_signaure.info.secret_info.load_regions[regions_count].memory_size = memory_size.u32.lo;

        if (0 == program_header_table[n].p_filesz) {
            // this is a zero-initialized segment
            esperanto_image_header->info.image_info_and_signaure.info.secret_info.load_regions[regions_count].region_offset = 0;
        } else {
            // this is a file-initialized segment
            if (program_header_table[n].p_offset < sizeof(ELF_HEADER_TYPE) || (program_header_table[n].p_offset + program_header_table[n].p_filesz) > image_length) {
                fprintf(stderr, "Error in %s: region %u file offset or size is invalid!\n", __func__, n);
                return -1;
            }
            esperanto_image_header->info.image_info_and_signaure.info.secret_info.load_regions[regions_count].region_flags |= ESPERANTO_EXECUTABLE_LOAD_REGION_FLAGS_LOAD;
            total_load_size = total_load_size + load_size.u32.lo;
            if (total_load_size < load_size.u32.lo) {
                fprintf(stderr, "Error in %s: total load size value overflow!\n", __func__);
                return -1;
            }
            // in this for loop we will store the offset of the region data (in the original ELF file) in load_regions[regions_count].region_offset
            // we will replace that in the next for loop with the actual offset value (in the signed ELF file)
            esperanto_image_header->info.image_info_and_signaure.info.secret_info.load_regions[regions_count].region_offset = (uint32_t)program_header_table[n].p_offset;
        }
        
        regions_count++;
    } // for

    // allocate memory for the code/data
    printf("total load size: %u (0x%x)\n", total_load_size, total_load_size);
    printf("total padding: %u (0x%x)\n", load_size_adjustement, load_size_adjustement);
    printf("total alloc size: %u (0x%x)\n", total_load_size + load_size_adjustement, total_load_size + load_size_adjustement);
    load_data = (uint8_t*)malloc(total_load_size + load_size_adjustement);
    if (NULL == load_data) {
        fprintf(stderr, "Error in %s: malloc() failed!\n", __func__);
        return -1;
    }
    memset(load_data, 0, total_load_size + load_size_adjustement);

    // scan the ELF progam header table for the second time
    offset = 0;
    for (n = 0; n < program_header_table_size; n++) {
        if (ESPERANTO_EXECUTABLE_LOAD_REGION_FLAGS_LOAD & esperanto_image_header->info.image_info_and_signaure.info.secret_info.load_regions[n].region_flags) {
            memcpy(load_data + offset, image + esperanto_image_header->info.image_info_and_signaure.info.secret_info.load_regions[n].region_offset, esperanto_image_header->info.image_info_and_signaure.info.secret_info.load_regions[n].load_size);
        	esperanto_image_header->info.image_info_and_signaure.info.secret_info.load_regions[n].region_offset = offset; // update the offset of data
            offset = offset + esperanto_image_header->info.image_info_and_signaure.info.secret_info.load_regions[n].load_size;
            if (0 != (esperanto_image_header->info.image_info_and_signaure.info.secret_info.load_regions[n].load_size & 0x7F)) {
                increment = 128 - (esperanto_image_header->info.image_info_and_signaure.info.secret_info.load_regions[n].load_size & 0x7F);
                offset += increment;
                esperanto_image_header->info.image_info_and_signaure.info.secret_info.load_regions[n].load_size += increment;
            }
        }
    }

    if (offset != (total_load_size + load_size_adjustement)) {
        fprintf(stderr, "Error: load size adjustement mismatch!\n");
        return -1;
    }

    esperanto_image_header->info.image_info_and_signaure.info.secret_info.load_regions_count = regions_count;
    *esperanto_image_code_and_data_length = total_load_size + load_size_adjustement;
    *esperanto_image_code_and_data = load_data;
    load_data = NULL;
    rval = 0;

    if (NULL != load_data) {
        free(load_data);
    }

    return rval;
}

//static int parse_elf32(const uint8_t * image, uint32_t image_length, uint8_t ** pesperanto_image_code_and_data, uint32_t * pesperanto_image_code_and_data_length, ESPERANTO_IMAGE_FILE_HEADER_t * pesperanto_image_header) {
static int HEADER_PARSE_FN( // parse_elfXX_header
    const uint8_t * image, 
    uint32_t image_length, 
    uint8_t ** pesperanto_image_code_and_data, 
    uint32_t * pesperanto_image_code_and_data_length, 
    ESPERANTO_IMAGE_FILE_HEADER_t * pesperanto_image_header,
    uint32_t * image_info_addr_lo,
    uint32_t * image_info_addr_hi,
    uint32_t * image_info_symbol_size
) {
    const ELF_HEADER_TYPE * header; // ElfXX_Ehdr *
    const ELF_PH_TABLE_TYPE * program_header_table;
    uint32_t program_header_table_size;
    uint32_t program_header_table_end_offset;
    const ELF_SH_TABLE_TYPE * section_header_table;
    uint32_t section_header_table_size;
    uint32_t section_header_table_end_offset; 
    U_64_32_t address;

    if (NULL == image || image_length < sizeof(ELF_HEADER_TYPE) || NULL == pesperanto_image_code_and_data || NULL == pesperanto_image_code_and_data_length || NULL == pesperanto_image_header) {
        fprintf(stderr, "Error in %s: invalid arguments!\n", __func__);
        return -1;
    }

    header = (const ELF_HEADER_TYPE *)image;

    if (ET_EXEC != header->e_type) {
        fprintf(stderr, "Error in %s: e_type is not ET_EXEC!\n", __func__);
        return -1;
    }

    if (EM_RISCV != header->e_machine) {
        fprintf(stderr, "Error in %s: e_machine is not EM_RISCV!\n", __func__);
        return -1;
    }

    if (EV_CURRENT != header->e_version) {
        fprintf(stderr, "Error in %s: e_version is not EV_CURRENT!\n", __func__);
        return -1;
    }

    // validate the program header table
    if (sizeof(ELF_PH_TABLE_TYPE) != header->e_phentsize) {
        fprintf(stderr, "Error in %s: e_phentsize value is not %lu!\n", __func__, sizeof(ELF_PH_TABLE_TYPE));
        return -1;
    }
    program_header_table_size = (uint32_t)(header->e_phnum * header->e_phentsize);
    program_header_table_end_offset = (uint32_t)(header->e_phoff + program_header_table_size);
    if (header->e_phoff < sizeof(ELF_HEADER_TYPE) || image_length < program_header_table_end_offset) {
        fprintf(stderr, "Error in %s: Invalid program header table offset or size!\n", __func__);
        return -1;
    }

    // scan the program header table
    program_header_table = (const ELF_PH_TABLE_TYPE *)(image + header->e_phoff);
    if (0 != PH_TABLE_PARSE_FN(program_header_table, header->e_phnum, image, image_length, pesperanto_image_code_and_data, pesperanto_image_code_and_data_length, pesperanto_image_header)) {
        fprintf(stderr, "Error in %s: failed to parse ELF program header table!\n", __func__);
        return -1;
    }

    // set the start address
    address.u64 = header->e_entry;
    pesperanto_image_header->info.image_info_and_signaure.info.secret_info.exec_address_lo = address.u32.lo;
    pesperanto_image_header->info.image_info_and_signaure.info.secret_info.exec_address_hi = address.u32.hi;

    // validate the section header table
    if (sizeof(ELF_SH_TABLE_TYPE) != header->e_shentsize) {
        fprintf(stderr, "Error in %s: e_shentsize value is not %lu!\n", __func__, sizeof(ELF_SH_TABLE_TYPE));
        return -1;
    }
    section_header_table_size = (uint32_t)(header->e_shnum * header->e_shentsize);
    section_header_table_end_offset = (uint32_t)(header->e_shoff + section_header_table_size);
    if (header->e_shoff < sizeof(ELF_HEADER_TYPE) || image_length < section_header_table_end_offset) {
        fprintf(stderr, "Error in %s: Invalid program header table offset or size!\n", __func__);
        return -1;
    }

    // parse the section header table to find the IMAGE_VERSION_INFO_t g_image_version_info symbol
    section_header_table = (const ELF_SH_TABLE_TYPE *)(image + header->e_shoff);
    FIND_IMAGE_VERSION_INFO_FN(section_header_table, header->e_shnum, image, image_length, image_info_addr_lo, image_info_addr_hi, image_info_symbol_size);

    return 0;
}
