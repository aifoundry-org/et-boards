#ifndef __ESPERANTO_LOAD_ELF_H__
#define __ESPERANTO_LOAD_ELF_H__

#include "crypto_api.h"

int load_elf_file(const char * elf_file_path, uint8_t ** esperanto_image_code_and_data, uint32_t * esperanto_image_code_and_data_length, ESPERANTO_IMAGE_FILE_HEADER_t * esperanto_image_header, const IMAGE_VERSION_INFO_t ** p_image_info);

#endif // __ESPERANTO_LOAD_ELF_H__
