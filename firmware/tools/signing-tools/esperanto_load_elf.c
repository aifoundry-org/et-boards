#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <argp.h>
#include <stddef.h>

#include "crypto_api.h"
#include "crypto_api_load_file.h"

#include "elf.h"

#include "esperanto_sign_elf.h"
#include "esperanto_load_elf.h"

typedef union {
    uint64_t u64;
    struct {
        uint32_t lo;
        uint32_t hi;
    } u32;
} U_64_32_t;

#define FUNCTION_NAME(prefix, postfix) prefix##_elf64_##postfix
#define TYPE_NAME(name) Elf64_##name
#define VAR_NAME(name) elf64_##name
#define ELF_ST_BIND ELF64_ST_BIND
#define ELF_ST_TYPE ELF64_ST_TYPE
#define ELF_ST_INFO ELF64_ST_INFO
#include "esperanto_load_elf_impl.h"
#undef ELF_ST_INFO
#undef ELF_ST_TYPE
#undef ELF_ST_BIND
#undef VAR_NAME
#undef TYPE_NAME
#undef FUNCTION_NAME

#define FUNCTION_NAME(prefix, postfix) prefix##_elf32_##postfix
#define TYPE_NAME(name) Elf32_##name
#define VAR_NAME(name) elf32_##name
#define ELF_ST_BIND ELF32_ST_BIND
#define ELF_ST_TYPE ELF32_ST_TYPE
#define ELF_ST_INFO ELF32_ST_INFO
#include "esperanto_load_elf_impl.h"
#undef ELF_ST_INFO
#undef ELF_ST_TYPE
#undef ELF_ST_BIND
#undef VAR_NAME
#undef TYPE_NAME
#undef FUNCTION_NAME

static int verify_image_info(const uint8_t * esperanto_image_code_and_data, uint32_t esperanto_image_code_and_data_length, const ESPERANTO_IMAGE_FILE_HEADER_t * esperanto_image_header, const IMAGE_VERSION_INFO_t ** p_image_info, uint64_t symbol_address, uint32_t image_info_symbol_size) {
    uint32_t n;
    U_64_32_t region_address;
    uint64_t region_end_address;
    uint64_t symbol_end_address = symbol_address + sizeof(IMAGE_VERSION_INFO_t);
    const IMAGE_VERSION_INFO_t * image_info = NULL;
    uint32_t file_offset, file_offset_end;

    // verify if the g_image_version_info symbol is located in the code/data section
    if (0 == (IMAGE_INFO_SECRET_EXEC_FLAGS_64BIT & esperanto_image_header->info.image_info_and_signaure.info.secret_info.exec_flags)) {
        if (symbol_address & 0xFFFFFFFF00000000l) {
            // g_image_info_address is > 4GB even though it is a 32-bit image!
            return -1;
        }
    }

    if (0 == image_info_symbol_size) {
        fprintf(stderr, "Warning in verify_image_info(): image_info symbol size is undefined!\n");
    } else {
        if (sizeof(IMAGE_VERSION_INFO_t) != image_info_symbol_size) {
            fprintf(stderr, "Error in verify_image_info(): image_info symbol size is not valid!\n");
            return -1;
        }
    }
    for (n = 0; n < esperanto_image_header->info.image_info_and_signaure.info.secret_info.load_regions_count; n++) {
        region_address.u32.lo = esperanto_image_header->info.image_info_and_signaure.info.secret_info.load_regions[n].load_address_lo;
        region_address.u32.hi = esperanto_image_header->info.image_info_and_signaure.info.secret_info.load_regions[n].load_address_hi;
        region_end_address = region_address.u64 + esperanto_image_header->info.image_info_and_signaure.info.secret_info.load_regions[n].load_size;

        if (symbol_address < region_address.u64 || region_end_address < symbol_end_address) {
            continue;
        }

        file_offset = (uint32_t)(esperanto_image_header->info.image_info_and_signaure.info.secret_info.load_regions[n].region_offset + (symbol_address - region_address.u64));
        file_offset_end = (uint32_t)(file_offset + sizeof(IMAGE_VERSION_INFO_t));
        if (file_offset_end > esperanto_image_code_and_data_length) {
            fprintf(stderr, "Error in verify_image_info(): Invalid image_info symbol file offset/size!\n");
            return -1;
        }
        image_info = (const IMAGE_VERSION_INFO_t*)(esperanto_image_code_and_data + file_offset);
        break;
    }
    if (NULL == image_info) {
        fprintf(stderr, "Error in verify_image_info(): Invalid image_info symbol address does not match any load regions!\n");
        return -1;
    }
    if (image_info->prolog_tag != IMAGE_VERSION_INFO_PROLOG_TAG) {
        fprintf(stderr, "Error in verify_image_info(): Invalid image_info prolog tag is not valid!\n");
        return -1;
    }
    if (image_info->epilog_tag != IMAGE_VERSION_INFO_EPILOG_TAG) {
        fprintf(stderr, "Error in verify_image_info(): Invalid image_info epilog tag is not valid!\n");
        return -1;
    }

    *p_image_info = image_info;
    return 0;
}

int load_elf_file(const char * elf_file_path, uint8_t ** esperanto_image_code_and_data, uint32_t * esperanto_image_code_and_data_length, ESPERANTO_IMAGE_FILE_HEADER_t * esperanto_image_header, const IMAGE_VERSION_INFO_t ** p_image_info) {
    int rval;
    uint32_t n;
    char * data = NULL;
    size_t data_length;
    uint8_t * image = NULL;
    uint32_t image_length;
    bool image64;
    U_64_32_t image_info_address;
    uint32_t image_info_symbol_size;
    
    if (NULL == elf_file_path || NULL == esperanto_image_header || NULL == esperanto_image_code_and_data || NULL == esperanto_image_code_and_data_length) {
        fprintf(stderr, "Error in load_elf_file: invalid arguments!\n");
        rval = -1;
        goto DONE;
    }

    memset(&esperanto_image_header->info.image_info_and_signaure.info, 0, sizeof(esperanto_image_header->info.image_info_and_signaure.info));

    if (0 != crypto_api_load_file(elf_file_path, (char**)&data, &data_length)) {
        fprintf(stderr, "Error in load_elf_file: crypto_api_load_file() failed!\n");
        rval = -1;
        goto DONE;
    }
    image = (uint8_t*)data;
    image_length = (uint32_t)data_length;

    if (image_length < EI_NIDENT  ||
        ELFMAG0 != image[EI_MAG0] ||
        ELFMAG1 != image[EI_MAG1] ||
        ELFMAG2 != image[EI_MAG2] ||
        ELFMAG3 != image[EI_MAG3]) {
        fprintf(stderr, "Error in load_elf_file: not a valid ELF image!\n");
        rval = -1;
        goto DONE;
    }

    switch (image[EI_CLASS]) {
    case ELFCLASS32:
        image64 = false;
        break;
    case ELFCLASS64:
        image64 = true;
        break;
    default:
        fprintf(stderr, "Error in load_elf_file: Invalid ELF image EI_CLASS value!\n");
        rval = -1;
        goto DONE;
    }

    switch (image[EI_DATA]) {
    case ELFDATA2LSB:
        break;
    case ELFDATA2MSB:
        fprintf(stderr, "Error in load_elf_file: ELF files with MSB data encoding are not supported!\n");
        rval = -1;
        goto DONE;
    default:
        fprintf(stderr, "Error in load_elf_file: Invalid EI_DATA value!\n");
        rval = -1;
        goto DONE;
    }

    if (EV_CURRENT != image[EI_VERSION]) {
        fprintf(stderr, "Error in load_elf_file: Invalid EI_VERSION value!\n");
        rval = -1;
        goto DONE;
    }

    for (n = EI_PAD; n < EI_NIDENT; n++) {
        if (0 != image[n]) {
            fprintf(stderr, "Error in load_elf_file: Padding bytes should be zero!\n");
            rval = -1;
            goto DONE;
        }
    }

    if (image64) {
        if (0 != parse_elf64_header(image, image_length, esperanto_image_code_and_data, esperanto_image_code_and_data_length, esperanto_image_header, &image_info_address.u32.lo, &image_info_address.u32.hi, &image_info_symbol_size)) {
            fprintf(stderr, "Error in load_elf_file: parse_elf64() failed!\n");
            rval = -1;
            goto DONE;
        }
        esperanto_image_header->info.image_info_and_signaure.info.secret_info.exec_flags |= IMAGE_INFO_SECRET_EXEC_FLAGS_64BIT;
    } else {
        if (0 != parse_elf32_header(image, image_length, esperanto_image_code_and_data, esperanto_image_code_and_data_length, esperanto_image_header, &image_info_address.u32.lo, &image_info_address.u32.hi, &image_info_symbol_size)) {
            fprintf(stderr, "Error in load_elf_file: parse_elf32() failed!\n");
            rval = -1;
            goto DONE;
        }
    }

    if (0 != image_info_address.u64) {
        verify_image_info(*esperanto_image_code_and_data, *esperanto_image_code_and_data_length, esperanto_image_header, p_image_info, image_info_address.u64, image_info_symbol_size);
    }

    rval = 0;

DONE:
    if (NULL != image) {
        free(image);
    }

    return rval;
}
