#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <json-c/json.h>

#include "crypto_api.h"
#include "crypto_api_parse_json.h"

#include "crypto_api_pkcs11.h"

#define JSON_MODULE_KEY "module"
#define JSON_MODULE_PATH_KEY "path"
#define JSON_SESSION_KEY "session"
#define JSON_SESSION_SLOT_KEY "slot"
#define JSON_SESSION_FLAGS_KEY "flags"
#define JSON_SESSION_USERS_KEY "users"
#define JSON_SESSION_USER_TYPE_KEY "type"
#define JSON_SESSION_USER_PIN_KEY "pin"
#define JSON_OBJECT_KEY "object"
#define JSON_OBJECT_ATTRIBUTES_KEY "attributes"
#define JSON_OBJECT_ATTRIBUTE_TYPE_KEY "type"
#define JSON_OBJECT_ATTRIBUTE_VALUE_KEY "value"
#define JSON_OBJECT_ATTRIBUTE_STR_VALUE_KEY "str_value"
#define JSON_OBJECT_ATTRIBUTE_HEX_VALUE_KEY "hex_value"

static int hex2int(char hex, uint8_t * val) {
    if (NULL == val) {
        return -1;
    }

    if ('0' <= hex && hex <= '9') {
        *val = (uint8_t)(hex - '0');
        return 0;
    } else if ('a' <= hex && hex <= 'f') {
        *val = (uint8_t)(hex - 'a' + 10);
        return 0;
    } else if ('A' <= hex && hex <= 'F') {
        *val = (uint8_t)(hex - 'A' + 10);
        return 0;
    } else {
        return -1;
    }
}

static int convert_hexblob(const char * hex_str, const uint32_t hex_len, uint8_t * buffer, uint32_t buffer_size, uint32_t * length) {
    uint32_t bytes;
    uint32_t index;
    const char * hex_str_end;
    uint8_t lo, hi;

    if (NULL == hex_str || 0 == hex_len) {
        return -1; // missing arguments
    }
    if (NULL == buffer) {
        if (NULL == length) {
            return -1; // missing arguments
        }
    } else if (0 == buffer_size) {
        return -1; // missing arguments
    }
    if (0 != (hex_len & 0x1)) {
        return -2; // hex string must have an even number of digits
    }

    bytes = hex_len >> 1;
    *length = bytes;
    if (NULL != buffer) {
        if (bytes > buffer_size) {
            return -3;
        }
    }

    index = 0;
    hex_str_end = hex_str + hex_len;
    while (hex_str < hex_str_end) {
        if (0 != hex2int(*hex_str, &hi)) {
            return -4;
        }
        hex_str++;
        if (0 != hex2int(*hex_str, &lo)) {
            return -4;
        }
        hex_str++;

        if (NULL != buffer) {
            buffer[index] = (uint8_t)(lo | (hi << 4u));
        }
        index++;
    }

    return 0;
}

int crypto_api_convert_hexblob(const char * hex_str, const uint32_t hex_len, uint8_t * buffer, uint32_t buffer_size, uint32_t * length) {
    return convert_hexblob(hex_str, hex_len, buffer, buffer_size, length);
}

static int get_hexblob_value(json_object * jobj, uint8_t * buffer, uint32_t buffer_size, uint32_t * length, const char * error_message_prefix) {
    const char * hex_str;
    int hex_len;

    if (json_type_string != json_object_get_type(jobj)) {
        if (NULL != error_message_prefix) {
            fprintf(stderr, "%s is not a string type!", error_message_prefix);
        }
        return -1;
    }

    hex_str = json_object_get_string(jobj);
    if (NULL == hex_str) {
        if (NULL != error_message_prefix) {
            fprintf(stderr, "%s is a NULL string!", error_message_prefix);
        }
        return -1;
    }

    hex_len = json_object_get_string_len(jobj);

    switch (convert_hexblob(hex_str, (uint32_t)hex_len, buffer, buffer_size, length)) {
    case 0:
        return 0;
    
    case -2:
        if (NULL != error_message_prefix) {
            fprintf(stderr, "%s is an odd-length string!", error_message_prefix);
        }
        return -1;

    case -3:
        if (NULL != error_message_prefix) {
            fprintf(stderr, "%s string is too long!", error_message_prefix);
        }
        return -1;

    default:
        if (NULL != error_message_prefix) {
            fprintf(stderr, "%s is an invalid string!", error_message_prefix);
        }
        return -1;
    }
}

static int get_uinteger_value(json_object * jobj, uint32_t * value, const char * error_message_prefix) {
    const char * str;
    char * endptr;
    enum json_type type;
    long int intval;

    type = json_object_get_type(jobj);
    switch (type) {
    default:
        if (NULL != error_message_prefix) {
            fprintf(stderr, "%s is not supported type!", error_message_prefix);
        }
        return -1;
    case json_type_int:
        *value = (uint32_t)json_object_get_int(jobj);
        break;
    case json_type_string: 
        str = json_object_get_string(jobj);
        if (NULL == str || 0 == *str) {
            if (NULL != error_message_prefix) {
                fprintf(stderr, "%s is invalid or empty string!", error_message_prefix);
            }
            return -2;
        }
        intval = strtol(str, &endptr, 0);
        if (0 != *endptr) {
            if (NULL != error_message_prefix) {
                fprintf(stderr, "%s is not a valid integer value!", error_message_prefix);
            }
            return -3;
        }
        *value = (uint32_t)intval;
        break;
    }
    return 0;
}

int crypto_api_json_get_module_info(json_object * jobj, PKCS_MODULE_INFO_t ** pmodule_info) {
    int rval;
    json_object * module_obj;
    json_object * path_obj;
    PKCS_MODULE_INFO_t * module_info = NULL;
    const char * str;
    int str_len;

    if (NULL == jobj || NULL == pmodule_info) {
        fprintf(stderr, "ERROR in crypto_api_json_get_module_info: INVALID ARGUMENTS!\n");
        rval = -1;
        goto FAILED1;
    }

    module_info = (PKCS_MODULE_INFO_t*)malloc(sizeof(PKCS_MODULE_INFO_t));
    if (NULL == module_info) {
        fprintf(stderr, "ERROR in crypto_api_json_get_module_info: malloc(module_info) failed!\n");
        rval = -1;
        goto FAILED1;
    }
    module_info->module = NULL;

    if (TRUE != json_object_object_get_ex(jobj, JSON_MODULE_KEY, &module_obj)) {
        fprintf(stderr, "ERROR in crypto_api_json_get_module_info: missing key '" JSON_MODULE_KEY "'!\n");
        rval = -1;
        goto FAILED1;
    }

    if (json_type_object != json_object_get_type(module_obj)) {
        fprintf(stderr, "ERROR in crypto_api_json_get_module_info: '" JSON_MODULE_KEY "' is not an object type!\n");
        rval = -1;
        goto FAILED1;
    }

    if (TRUE != json_object_object_get_ex(module_obj, JSON_MODULE_PATH_KEY, &path_obj)) {
        fprintf(stderr, "ERROR in crypto_api_json_get_module_info: missing module key '" JSON_MODULE_PATH_KEY "'!\n");
        rval = -1;
        goto FAILED1;
    }

    if (json_type_string != json_object_get_type(path_obj)) {
        fprintf(stderr, "ERROR in crypto_api_json_get_module_info: module '" JSON_MODULE_PATH_KEY "' is not a string type!\n");
        rval = -1;
        goto FAILED1;
    }

    str = json_object_get_string(path_obj);
    if (NULL == str) {
        fprintf(stderr, "ERROR in crypto_api_json_get_module_info: module '" JSON_MODULE_PATH_KEY "' string is NULL!\n");
        rval = -1;
        goto FAILED1;
    }

    str_len = json_object_get_string_len(path_obj);
    module_info->module = (char*)malloc((uint32_t)(str_len + 1));
    if (NULL == module_info->module) {
        fprintf(stderr, "ERROR in crypto_api_json_get_module_info: malloc(module_info->module) failed!\n");
        rval = -1;
        goto FAILED1;
    }
    strcpy(module_info->module, str);

    *pmodule_info = module_info;
    module_info = NULL;
    rval = 0;

FAILED1:
    crypto_api_free_module_info(module_info);

    return rval;
}

void crypto_api_free_module_info(PKCS_MODULE_INFO_t * module_info) {
    if (NULL != module_info) {
        if (NULL != module_info->module) {
            free(module_info->module);
        }
        free(module_info);
    }
}

int crypto_api_json_get_session_info(json_object * jobj, PKCS_SESSION_INFO_t ** psession_info) {
    int rval;
    int index, users_count;
    json_object * session_obj;
    json_object * users_obj;
    json_object * user_obj;
    json_object * value_obj;
    const char * str;
    int str_len;
    PKCS_SESSION_INFO_t * session_info = NULL;

    if (NULL == jobj || NULL == psession_info) {
        return -1;
    }

    session_info = (PKCS_SESSION_INFO_t*)malloc(sizeof(PKCS_SESSION_INFO_t));
    if (NULL == session_info) {
        fprintf(stderr, "ERROR in crypto_api_json_get_session_info: malloc(session_info) failed!\n");
        rval = -1;
        goto FAILED1;
    }
    session_info->users_count = 0;
    session_info->users = NULL;

    // find and check the session object

    if (TRUE != json_object_object_get_ex(jobj, JSON_SESSION_KEY, &session_obj)) {
        fprintf(stderr, "ERROR in crypto_api_json_get_session_info: missing key '" JSON_SESSION_KEY "'!\n");
        rval = -1;
        goto FAILED1;
    }

    if (json_type_object != json_object_get_type(session_obj)) {
        fprintf(stderr, "ERROR in crypto_api_json_get_session_info: '" JSON_SESSION_KEY "' is not an object type!\n");
        rval = -1;
        goto FAILED1;
    }

    // find and check the required slot_id

    if (TRUE != json_object_object_get_ex(session_obj, JSON_SESSION_SLOT_KEY, &value_obj)) {
        fprintf(stderr, "ERROR in crypto_api_json_get_session_info: missing key '" JSON_SESSION_SLOT_KEY "'!\n");
        rval = -1;
        goto FAILED1;
    }

    if (0 != get_uinteger_value(value_obj, &(session_info->slot_id), "ERROR in crypto_api_json_get_session_info: SESSION '" JSON_SESSION_SLOT_KEY "' ")) {
        rval = -1;
        goto FAILED1;
    }

    // find and check the optional flags flag

    if (TRUE == json_object_object_get_ex(session_obj, JSON_SESSION_FLAGS_KEY, &value_obj)) {
        if (0 != get_uinteger_value(value_obj, &(session_info->flags), "ERROR in crypto_api_json_get_session_info: SESSION '" JSON_SESSION_FLAGS_KEY "' ")) {
            rval = -1;
            goto FAILED1;
        }
    } else {
        session_info->flags = 0;
    }

    // find and parse the optional users list

    if (TRUE == json_object_object_get_ex(session_obj, JSON_SESSION_USERS_KEY, &users_obj)) {
        if (json_type_array != json_object_get_type(users_obj)) {
            fprintf(stderr, "ERROR in crypto_api_json_get_session_info: SESSION '" JSON_SESSION_USERS_KEY "' is not a array type!\n");
            rval = -1;
            goto FAILED1;
        }

        users_count = json_object_array_length(users_obj);
        if (0 == users_count) {
            fprintf(stderr, "ERROR in crypto_api_json_get_session_info: SESSION '" JSON_SESSION_USERS_KEY "' array is empty!\n");
            rval = -1;
            goto FAILED1;
        }

        session_info->users = malloc((uint32_t)users_count * sizeof(PKCS_USER_INFO_t));
        if (NULL == session_info->users) {
            fprintf(stderr, "ERROR in crypto_api_json_get_session_info: malloc(session_info->users) failed!\n");
            rval = -1;
            goto FAILED1;
        }
        memset(session_info->users, 0, (uint32_t)users_count * sizeof(PKCS_USER_INFO_t));

        // iterate all users

        for (index = 0; index < users_count; index++) {
            user_obj = json_object_array_get_idx(users_obj, index);
            if (NULL == user_obj) {
                fprintf(stderr, "ERROR in crypto_api_json_get_session_info: missing SESSION '" JSON_SESSION_USERS_KEY "' array element #%d!\n", index);
                rval = -1;
                goto FAILED1;
            }

            if (json_type_object != json_object_get_type(user_obj)) {
                fprintf(stderr, "ERROR in crypto_api_json_get_session_info: SESSION '" JSON_SESSION_USERS_KEY "' array element #%d is not an object type!\n", index);
                rval = -1;
                goto FAILED1;
            }

            session_info->users[index].user_type_valid = 0;
            session_info->users[index].user_pin_valid = 0;
            
            // find and parse optional user type
            if (TRUE == json_object_object_get_ex(user_obj, JSON_SESSION_USER_TYPE_KEY, &value_obj)) {
                if (0 != get_uinteger_value(value_obj, &(session_info->users[index].user_type), "ERROR in crypto_api_json_get_session_info: SESSION USER'" JSON_SESSION_USER_TYPE_KEY "' ")) {
                    rval = -1;
                    goto FAILED1;
                }
                session_info->users[index].user_type_valid = 1;
            }

            // find and parse optional user pin
            if (TRUE == json_object_object_get_ex(user_obj, JSON_SESSION_USER_PIN_KEY, &value_obj)) {
                if (json_type_string != json_object_get_type(value_obj)) {
                    fprintf(stderr, "ERROR in crypto_api_json_get_session_info: SESSION USER '" JSON_SESSION_USER_PIN_KEY "' is not a string type!\n");
                    rval = -1;
                    goto FAILED1;
                }

                str = json_object_get_string(value_obj);
                if (NULL == str) {
                    fprintf(stderr, "ERROR in crypto_api_json_get_session_info: SESSION USER '" JSON_SESSION_USER_PIN_KEY "' string is NULL!\n");
                    rval = -1;
                    goto FAILED1;
                }

                str_len = json_object_get_string_len(value_obj);
                if (str_len >= 0) {
                    session_info->users[index].user_pin = (char*)malloc((uint32_t)str_len + 1);
                    if (NULL == session_info->users[index].user_pin) {
                        fprintf(stderr, "ERROR in crypto_api_json_get_session_info: malloc(session_info->users[].user_pin) failed!\n");
                        rval = -1;
                        goto FAILED1;
                    }
                    strcpy(session_info->users[index].user_pin, str);
                }
                session_info->users[index].user_pin_len = (uint32_t)str_len;

                session_info->users[index].user_pin_valid = 1;
            }

            if (0 == session_info->users[index].user_type_valid && 0 == session_info->users[index].user_pin_valid) {
                fprintf(stderr, "ERROR in crypto_api_json_get_session_info: SESSION USER '" JSON_SESSION_USER_TYPE_KEY "' and '" JSON_SESSION_USER_PIN_KEY "'are missing!\n");
                rval = -1;
                goto FAILED1;
            }
        }

        session_info->users_count = (uint32_t)users_count;
    }

    *psession_info = session_info;
    session_info = NULL;
    rval = 0;

FAILED1:
    crypto_api_free_session_info(session_info);

    return rval;
}

void crypto_api_free_session_info(PKCS_SESSION_INFO_t * session_info) {
    if (NULL != session_info) {
        if (NULL != session_info->users) {
            free(session_info->users);
        }
        free(session_info);
    }
}
/*
static int convert_integer_id(const uint32_t integer_id, uint8_t * id_buffer, uint32_t id_buffer_size, uint32_t * id_length) {
    uint32_t bytes_needed = 0;
    uint32_t temp = integer_id;

    if (NULL == id_buffer && NULL == id_length) {
        return -1;
    } else if (NULL != id_buffer && 0 == id_buffer_size) {
        return -1;
    }

    do {
        if (NULL != id_buffer) {
            if (bytes_needed >= id_buffer_size) {
                return -1;
            }
            id_buffer[bytes_needed] = temp & 0xFF;
        }
        temp = temp >> 8u;
        bytes_needed++;
    } while (0 != temp);
    *id_length = bytes_needed;

    return 0;
}
*/

static int get_object_attribute_type(uint32_t * attribute_type, const char * attribute_type_string) {
    long intval;
    char * endptr;

    if (0 == *attribute_type_string) {
        return -1;
    }

    if (0 == pkcs11_string_to_attribute(attribute_type_string, attribute_type)) {
        return 0;
    }

    intval = strtol(attribute_type_string, &endptr, 0);
    if (0 != *endptr) {
        return -1;
    }
    *attribute_type = (uint32_t)intval;

    return 0;
}

static int crypto_api_json_get_attribute_info(uint32_t attribute_index, json_object * attribute_obj, PKCS_ATTRIBUTE_INFO_t * const attribute_info) {
    int rval;
    json_object * type_obj;
    json_object * value_obj;
    const char * attribute_type_string;
    uint32_t data_length;
    const char * str;
    uint32_t u32;

//    attribute_info->

    if (TRUE != json_object_object_get_ex(attribute_obj, JSON_OBJECT_ATTRIBUTE_TYPE_KEY, &type_obj)) {
        fprintf(stderr, "ERROR in crypto_api_json_get_attribute_info: missing object attribute %u key '" JSON_OBJECT_ATTRIBUTE_TYPE_KEY "'!\n", attribute_index);
        rval = 1;
        goto FAILED;
    }
    if (json_type_string != json_object_get_type(type_obj)) {
        fprintf(stderr, "ERROR in crypto_api_json_get_attribute_info: object attribute %u '" JSON_OBJECT_ATTRIBUTE_TYPE_KEY "' is not a string type!\n", attribute_index);
        rval = -1;
        goto FAILED;
    }
    attribute_type_string = json_object_get_string(type_obj);
    if (NULL == attribute_type_string) {
        fprintf(stderr, "ERROR in crypto_api_json_get_attribute_info: object attribute %u '" JSON_OBJECT_ATTRIBUTE_TYPE_KEY "' string is NULL!\n", attribute_index);
        rval = -1;
        goto FAILED;
    }
    if ('#' == *attribute_type_string) {
        // skip this attribute
        rval = 1;
        goto FAILED;
    }
    if (0 != get_object_attribute_type(&attribute_info->attribute_type, attribute_type_string)) {
        fprintf(stderr, "ERROR in crypto_api_json_get_attribute_info: object attribute %u '" JSON_OBJECT_ATTRIBUTE_TYPE_KEY "' is not valid!\n", attribute_index);
        rval = -1;
        goto FAILED;
    }
    if (TRUE == json_object_object_get_ex(attribute_obj, JSON_OBJECT_ATTRIBUTE_VALUE_KEY, &value_obj)) {
        if (json_type_boolean == json_object_get_type(value_obj)) {
            attribute_info->data_type = DATA_TYPE_BOOLEAN;
            attribute_info->data.boolean = json_object_get_boolean(value_obj) ? true : false;
        } else {
            attribute_info->data_type = DATA_TYPE_NUMBER;
            if (0 != get_uinteger_value(value_obj, &u32, "ERROR in crypto_api_json_get_attribute_info(%u): OBJECT ATTRIBUTE '" JSON_OBJECT_ATTRIBUTE_VALUE_KEY "' ")) {
                rval = -1;
                goto FAILED;
            }
            attribute_info->data.number = u32;
        }
    } else if (TRUE == json_object_object_get_ex(attribute_obj, JSON_OBJECT_ATTRIBUTE_STR_VALUE_KEY, &value_obj)) {
        attribute_info->data_type = DATA_TYPE_STRING;
        if (json_type_string != json_object_get_type(value_obj)) {
            fprintf(stderr, "ERROR in crypto_api_json_get_attribute_info: object attribute %u '" JSON_OBJECT_ATTRIBUTE_STR_VALUE_KEY "' is not a string type!\n", attribute_index);
            rval = -1;
            goto FAILED;
        }
        str = json_object_get_string(value_obj);
        if (NULL == str) {
            fprintf(stderr, "ERROR in crypto_api_json_get_attribute_info: object attribute %u '" JSON_OBJECT_ATTRIBUTE_STR_VALUE_KEY "' is a NULL string!\n", attribute_index);
            rval = -1;
            goto FAILED;
        }
        data_length = (uint32_t)strlen(str);
        attribute_info->data.string.p = (char*)malloc(data_length + 1);
        if (NULL == attribute_info->data.string.p) {
            fprintf(stderr, "ERROR in crypto_api_json_get_attribute_info: malloc() failed!\n");
            rval = -1;
            goto FAILED;
        }
        memcpy(attribute_info->data.string.p, str, data_length);
        attribute_info->data.string.len = data_length;
        attribute_info->data.string.p[data_length] = 0;
    } else if (TRUE == json_object_object_get_ex(attribute_obj, JSON_OBJECT_ATTRIBUTE_HEX_VALUE_KEY, &value_obj)) {
        attribute_info->data_type = DATA_TYPE_BLOB;
        if (0 != get_hexblob_value(value_obj, NULL, 0, &data_length, "ERROR in crypto_api_json_get_attribute_info(%u): OBJECT ATTRIBUTE '" JSON_OBJECT_ATTRIBUTE_HEX_VALUE_KEY "' ")) {
            fprintf(stderr, "ERROR in crypto_api_json_get_attribute_info: get_hexblob_value() failed!\n");
            rval = -1;
            goto FAILED;
        }
        attribute_info->data.blob.p = malloc(data_length);
        if (NULL == attribute_info->data.blob.p) {
            fprintf(stderr, "ERROR in crypto_api_json_get_attribute_info: malloc() failed!\n");
            rval = -1;
            goto FAILED;
        }
        attribute_info->data.blob.len = data_length;
        if (0 != get_hexblob_value(value_obj, attribute_info->data.blob.p, attribute_info->data.blob.len, &data_length, "ERROR in crypto_api_json_get_attribute_info(%u): OBJECT ATTRIBUTE '" JSON_OBJECT_ATTRIBUTE_HEX_VALUE_KEY "' ")) {
            fprintf(stderr, "ERROR in crypto_api_json_get_attribute_info: get_hexblob_value() failed!\n");
            free(attribute_info->data.blob.p);
            rval = -1;
            goto FAILED;
        }
    } else {
        fprintf(stderr, "ERROR in crypto_api_json_get_attribute_info: missing object attribute %u value!\n", attribute_index);
        rval = -1;
        goto FAILED;
    }

    rval = 0;

FAILED:
    return rval;
}

int crypto_api_json_get_object_info(json_object * jobj, PKCS_OBJECT_INFO_t ** pobject_info) {
    int rval;
    json_object * object_obj;
    json_object * attributes_obj;
    json_object * attribute_obj;
    uint32_t attributes_count;
    uint32_t attributes_valid_count;
    uint32_t attribute_index;
    PKCS_ATTRIBUTE_INFO_t * attributes_array;
    PKCS_OBJECT_INFO_t * object_info = NULL;

    if (NULL == jobj || NULL == pobject_info) {
        return -1;
    }

    object_info = (PKCS_OBJECT_INFO_t*)malloc(sizeof(PKCS_OBJECT_INFO_t));
    if (NULL == object_info) {
        fprintf(stderr, "ERROR in crypto_api_json_get_object_info: malloc(object_info) failed!\n");
        rval = -1;
        goto FAILED1;
    }
    object_info->attribute_count = 0;
    object_info->attributes = NULL;

    if (TRUE != json_object_object_get_ex(jobj, JSON_OBJECT_KEY, &object_obj)) {
        fprintf(stderr, "ERROR in crypto_api_json_get_object_info: missing key '" JSON_OBJECT_KEY "'!\n");
        rval = -1;
        goto FAILED1;
    }

    if (json_type_object != json_object_get_type(object_obj)) {
        fprintf(stderr, "ERROR in crypto_api_json_get_object_info: '" JSON_OBJECT_KEY "' is not an object type!\n");
        rval = -1;
        goto FAILED1;
    }

    // find and parse object attributes

    if (TRUE != json_object_object_get_ex(object_obj, JSON_OBJECT_ATTRIBUTES_KEY, &attributes_obj)) {
        fprintf(stderr, "ERROR in crypto_api_json_get_object_info: missing object key '" JSON_OBJECT_ATTRIBUTES_KEY "'!\n");
        rval = -1;
        goto FAILED1;
    }
    if (json_type_array != json_object_get_type(attributes_obj)) {
        fprintf(stderr, "ERROR in crypto_api_json_get_object_info: object '" JSON_OBJECT_ATTRIBUTES_KEY "' is not an array type!\n");
        rval = -1;
        goto FAILED1;
    }
    attributes_count = (uint32_t)json_object_array_length(attributes_obj);
    if (0 == attributes_count) {
        fprintf(stderr, "ERROR in crypto_api_json_get_object_info: object '" JSON_OBJECT_ATTRIBUTES_KEY "' array is empty!\n");
        rval = -1;
        goto FAILED1;
    }

    // allocate the attributes array
    
    attributes_array = (PKCS_ATTRIBUTE_INFO_t*)malloc(attributes_count * sizeof(PKCS_ATTRIBUTE_INFO_t));
    if (NULL == attributes_array) {
        fprintf(stderr, "ERROR in crypto_api_json_get_object_info: failed to allocate memory for attributes array!\n");
        rval = -1;
        goto FAILED1;
    }
    memset(attributes_array, 0, attributes_count * sizeof(PKCS_ATTRIBUTE_INFO_t));
    attributes_valid_count = 0;

    // find and parse each attribute in the array

    for (attribute_index = 0; attribute_index < attributes_count; attribute_index++) {
        attribute_obj = json_object_array_get_idx(attributes_obj, (int)attribute_index);
        if (NULL == attribute_obj) {
            fprintf(stderr, "ERROR in crypto_api_json_get_object_info: missing OBJECT '" JSON_OBJECT_ATTRIBUTES_KEY "' array element #%d!\n", attribute_index);
            rval = -1;
            goto FAILED2;
        }
        if (json_type_object != json_object_get_type(attribute_obj)) {
            fprintf(stderr, "ERROR in crypto_api_json_get_object_info: OBJECT '" JSON_OBJECT_ATTRIBUTES_KEY "' array element #%d is not an object type!\n", attribute_index);
            rval = -1;
            goto FAILED2;
        }

        rval = crypto_api_json_get_attribute_info(attribute_index, attribute_obj, &attributes_array[attributes_valid_count]);
        if (1 == rval) {
            // the attribute was commented out
            continue;
        } else if (0 != rval) {
            fprintf(stderr, "ERROR in crypto_api_json_get_object_info: crypto_api_json_get_attribute_info(%u) failed!\n", attribute_index);
            rval = -1;
            goto FAILED2;
        }

        attributes_valid_count++;
    }

    if(0 == attributes_valid_count) {
        fprintf(stderr, "ERROR in crypto_api_json_get_object_info: OBJECT '" JSON_OBJECT_ATTRIBUTES_KEY "' array does not contain any valid attributes!\n");
        rval = -1;
        goto FAILED2;
    }

    object_info->attribute_count = attributes_valid_count;
    object_info->attributes = attributes_array;
    *pobject_info = object_info;
    rval = 0;
    goto DONE;

FAILED2:
    for (attribute_index = 0; attribute_index < attributes_valid_count; attribute_index++) {
        switch (attributes_array[attribute_index].data_type) {
            case DATA_TYPE_STRING:
                if (NULL != attributes_array[attribute_index].data.string.p) {
                    free(attributes_array[attribute_index].data.string.p);
                }
                break;
            case DATA_TYPE_BLOB:
                if (NULL != attributes_array[attribute_index].data.blob.p) {
                    free(attributes_array[attribute_index].data.blob.p);
                }
                break;
            default:
                break;
        }
    }
    free(attributes_array);

FAILED1:
DONE:
    return rval;
}

void crypto_api_free_object_info(PKCS_OBJECT_INFO_t * object_info) {
    uint32_t attribute_index;
    if (NULL != object_info) {
        if (NULL != object_info->attributes) {
            for (attribute_index = 0; attribute_index < object_info->attribute_count; attribute_index++) {
                switch (object_info->attributes[attribute_index].data_type) {
                    case DATA_TYPE_STRING:
                        if (NULL != object_info->attributes[attribute_index].data.string.p) {
                            free(object_info->attributes[attribute_index].data.string.p);
                        }
                        break;
                    case DATA_TYPE_BLOB:
                        if (NULL != object_info->attributes[attribute_index].data.blob.p) {
                            free(object_info->attributes[attribute_index].data.blob.p);
                        }
                        break;
                    default:
                        break;
                }
            } // for
            free(object_info->attributes);
        } // if
        free(object_info);
    } // if
}
