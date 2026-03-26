#ifndef __CRYPTO_API_LOAD_FILE_H__
#define __CRYPTO_API_LOAD_FILE_H__

#include <stdint.h>
#include <stddef.h>

int crypto_api_load_file(const char * file_path, char ** buffer, size_t * buffer_size);
int crypto_api_load_certificate_request(ESPERANTO_CERTIFICATE_REQUEST_t * pcertificate, const char * certificate_file_path);
int crypto_api_load_certificate(ESPERANTO_CERTIFICATE_t * pcertificate, const char * certificate_file_path);
int crypto_api_load_signed_elf_file(ESPERANTO_IMAGE_FILE_HEADER_t ** header, const uint8_t ** code_and_data, uint32_t * code_and_data_size, const char * certificate_file_path);
int crypto_api_load_signed_raw_file(ESPERANTO_RAW_IMAGE_FILE_HEADER_t ** pheader, const uint8_t ** praw_data, uint32_t * praw_data_size, const char * signed_raw_file_path);

#endif
