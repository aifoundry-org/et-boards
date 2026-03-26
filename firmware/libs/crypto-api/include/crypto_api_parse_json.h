#ifndef __CRYPTO_API_PARSE_JSON_H__
#define __CRYPTO_API_PARSE_JSON_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <json-c/json.h>

int crypto_api_json_get_module_info(json_object * jobj, PKCS_MODULE_INFO_t ** pmodule_info);
int crypto_api_json_get_session_info(json_object * jobj, PKCS_SESSION_INFO_t ** psession_info);
int crypto_api_json_get_object_info(json_object * jobj, PKCS_OBJECT_INFO_t ** pobject_info);

void crypto_api_free_module_info(PKCS_MODULE_INFO_t * module_info);
void crypto_api_free_session_info(PKCS_SESSION_INFO_t * session_info);
void crypto_api_free_object_info(PKCS_OBJECT_INFO_t * object_info);

int crypto_api_convert_hexblob(const char * hex_str, const uint32_t hex_len, uint8_t * buffer, uint32_t buffer_size, uint32_t * length);

#endif
