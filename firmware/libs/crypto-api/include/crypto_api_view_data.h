#ifndef __CRYPTO_API_VIEW_DATA_H__
#define __CRYPTO_API_VIEW_DATA_H__

#include <stdint.h>
#include <stddef.h>

void crypto_api_dumphex(const void * data, uint32_t data_size, uint32_t max_line_length);
void crypto_api_dump_githash(bool error, const uint8_t hash[256/8]);

void crypto_api_dump_pkcs_module_info(const PKCS_MODULE_INFO_t * module_info);
void crypto_api_dump_pkcs_session_info(const PKCS_SESSION_INFO_t * session_info);
void crypto_api_dump_pkcs_object_info(const PKCS_OBJECT_INFO_t * object_info);

void crypto_api_view_date_time(const DATE_AND_TIME_STAMP_t * date_time);

int crypto_api_view_signature(const PUBLIC_SIGNATURE_t * signature, bool silent, bool verbose);
int crypto_api_view_certificate_request(const ESPERANTO_CERTIFICATE_REQUEST_t * pcertificate, bool silent, bool verbose);
int crypto_api_view_certificate(const ESPERANTO_CERTIFICATE_t * pcertificate, bool silent, bool verbose);
int crypto_view_signed_elf_header(const ESPERANTO_IMAGE_FILE_HEADER_t * header, bool silent, bool verbose, bool decrypted, bool display_image_header, bool display_file_header, bool do_not_check_signature);
int crypto_view_signed_raw_header(const ESPERANTO_RAW_IMAGE_FILE_HEADER_t * header, bool silent, bool verbose, bool display_image_header, bool display_file_header, bool do_not_check_signature);

#endif
