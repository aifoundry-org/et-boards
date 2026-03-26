#ifndef __CRYPTO_API_LOAD_JSON_H__
#define __CRYPTO_API_LOAD_JSON_H__

#include <stdint.h>
#include <stddef.h>

#include <json-c/json.h>

int crypto_api_load_json(const char * file_path, json_object ** p_jobj);

#endif
