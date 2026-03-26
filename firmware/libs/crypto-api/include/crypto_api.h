#ifndef __CRYPTO_API_H__
#define __CRYPTO_API_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "esperanto_signed_image_format/public_key_data.h"
#include "esperanto_signed_image_format/certificate_request.h"
#include "esperanto_signed_image_format/certificate.h"
#include "esperanto_signed_image_format/executable_image.h"
#include "esperanto_signed_image_format/raw_image.h"

typedef struct PKCS_MODULE_INFO {
    char * module;
} PKCS_MODULE_INFO_t;

typedef struct PKCS_USER_INFO {
    uint32_t user_type_valid;
    uint32_t user_type;
    uint32_t user_pin_valid;
    char * user_pin;
    uint32_t user_pin_len;
} PKCS_USER_INFO_t;

typedef struct PKCS_SESSION_INFO {
    uint32_t slot_id;
    uint32_t flags;
    uint32_t users_count;
    PKCS_USER_INFO_t * users;
} PKCS_SESSION_INFO_t;

typedef enum DATA_TYPE {
    DATA_TYPE_INVALID = 0,
    DATA_TYPE_BOOLEAN,
    DATA_TYPE_NUMBER,
    DATA_TYPE_STRING,
    DATA_TYPE_BLOB
} DATA_TYPE_t;

typedef struct PKCS_ATTRIBUTE_INFO {
    uint32_t attribute_type;
    DATA_TYPE_t data_type;
    union {
        bool boolean;
        uint64_t number;
        struct {
            uint32_t len;
            char * p;
        } string;
        struct {
            uint32_t len;
            uint8_t * p;
        } blob;
    } data;
} PKCS_ATTRIBUTE_INFO_t;

typedef struct PKCS_OBJECT_INFO {
    uint32_t attribute_count;
    PKCS_ATTRIBUTE_INFO_t * attributes;
} PKCS_OBJECT_INFO_t;

typedef void * MODULE_HANDLE_t;
typedef void * SESSION_HANDLE_t;
typedef void * OBJECT_HANDLE_t;

int crypto_api_init(void);
int crypto_api_cleanup(void);

int crypto_api_module_init(MODULE_HANDLE_t * pmodule_handle, const char * module_path, void * pInitArgs);
int crypto_api_module_cleanup(MODULE_HANDLE_t module_handle);

int crypto_api_session_open(SESSION_HANDLE_t * psession_handle, MODULE_HANDLE_t module_handle, uint32_t flags, uint32_t slot_id, const PKCS_USER_INFO_t * users, uint32_t users_count);
int crypto_api_session_close(SESSION_HANDLE_t session_handle);

int crypto_api_open_secret_key(OBJECT_HANDLE_t * pobject_handle, SESSION_HANDLE_t session_handle, const PKCS_OBJECT_INFO_t * pkcs_object_info);
int crypto_api_close_secret_key(OBJECT_HANDLE_t object_handle);
int crypto_api_generate_secret_key(void);
int crypto_api_import_secret_key(SESSION_HANDLE_t session_handle, const PKCS_OBJECT_INFO_t * pkcs_object_info, const uint8_t * key, uint32_t key_size);
int crypto_api_export_secret_key(OBJECT_HANDLE_t object_handle, uint8_t * key_buffer, uint32_t key_buffer_size, uint32_t * key_size);

int crypto_api_open_public_key(OBJECT_HANDLE_t * pobject_handle, SESSION_HANDLE_t session_handle, const PKCS_OBJECT_INFO_t * pkcs_object_info);
int crypto_api_close_public_key(OBJECT_HANDLE_t object_handle);

int crypto_api_open_private_key(OBJECT_HANDLE_t * pobject_handle, SESSION_HANDLE_t session_handle, const PKCS_OBJECT_INFO_t * pkcs_object_info);
int crypto_api_close_private_key(OBJECT_HANDLE_t object_handle);
int crypto_api_get_public_key(OBJECT_HANDLE_t object_handle, PUBLIC_KEY_t * public_key_data);

int crypto_api_sha_hash(HASH_ALG_t hash_algorithm, uint8_t * hash_buffer, size_t hash_buffer_size, uint32_t * hash_size, const void * data, size_t data_length);
int crypto_api_sha_hmac(void);

int crypto_api_aes_ecb_encrypt_init(OBJECT_HANDLE_t * secret_key_handle);
int crypto_api_aes_ecb_encrypt_update(OBJECT_HANDLE_t * secret_key_handle, const void * src, size_t src_size, void * dst, size_t * dst_size);
int crypto_api_aes_ecb_encrypt_final(OBJECT_HANDLE_t * secret_key_handle, void * dst, size_t * dst_size);

int crypto_api_aes_encrypt(void * dst, const void * src, size_t size, void * IV, OBJECT_HANDLE_t * secret_key_handle);
int crypto_api_aes_decrypt(void);

int crypto_api_aes_cmac(void);

int crypto_api_pk_sign(PUBLIC_SIGNATURE_t * signature, const void * data, size_t data_size, HASH_ALG_t hash_algorithm, OBJECT_HANDLE_t private_key_handle);
int crypto_api_pk_verify(const PUBLIC_SIGNATURE_t * signature, const void * data, size_t data_size, OBJECT_HANDLE_t public_key_handle, bool * signature_ok);
int crypto_api_verify_with_public_key(const PUBLIC_SIGNATURE_t * signature, const void * data, size_t data_size, const PUBLIC_KEY_t * public_key, bool * signature_ok);

int crypto_api_rsa_encrypt(void);
int crypto_api_rsa_decrypt(void);

int crypto_api_random(SESSION_HANDLE_t session_handle, void * ptr, size_t size);

EC_KEY_CURVE_ID_t ec_curve_name_to_id(const void * const name, const size_t name_length);
const char * ec_curve_id_to_name(EC_KEY_CURVE_ID_t curve_id);

HASH_ALG_t hash_algorithm_name_to_id(const char * name, const size_t name_length);
const char * hash_algorithm_to_name(HASH_ALG_t hash_algorithm, uint32_t * hash_size);

ESPERANTO_MAC_TYPE_t mac_algorithm_name_to_id(const char * name, const size_t name_length);
const char * mac_algorithm_to_name(ESPERANTO_MAC_TYPE_t mac_algorithm, uint32_t * mac_size);

int crypto_api_compute_key_identifier(KEY_IDENTIFIER_t * key_dientifier, const PUBLIC_KEY_t * public_key);

int crypto_api_get_current_date_and_time(DATE_AND_TIME_STAMP_t * datetime);

ESPERANTO_IMAGE_TYPE_t executable_image_type_name_to_type(const char * name, const size_t name_length);
const char * executable_image_type_to_type_name(ESPERANTO_IMAGE_TYPE_t image_type);

ESPERANTO_RAW_IMAGE_TYPE_t raw_image_type_name_to_type(const char * name, const size_t name_length);
const char * raw_image_type_to_type_name(ESPERANTO_RAW_IMAGE_TYPE_t image_type);

int pkcs11_string_to_attribute(const char * string, uint32_t * attribute);
const char * pkcs11_attribute_to_string(uint32_t attribute);
typedef enum PKCS11_DATA_TYPE_e {
    PKCS11_DATA_TYPE_INVALID = 0,
    PKCS11_DATA_TYPE_CK_OBJECT_CLASS,
    PKCS11_DATA_TYPE_CK_KEY_TYPE,
    PKCS11_DATA_TYPE_RFC_2279_STRING,
    PKCS11_DATA_TYPE_CK_BBOOL,
    PKCS11_DATA_TYPE_CK_ULONG,
    PKCS11_DATA_TYPE_CK_ID,
    PKCS11_DATA_TYPE_BYTE_ARRAY,
    PKCS11_DATA_TYPE_OTHER
} PKCS11_DATA_TYPE_t;

PKCS11_DATA_TYPE_t pkcs11_attribute_type(uint32_t attribute);

void diagnostics(void);

#endif // __CRYPTO_API_H__

