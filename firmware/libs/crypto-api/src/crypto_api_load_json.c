#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <json-c/json.h>

#include "crypto_api.h"
#include "crypto_api_load_file.h"
#include "crypto_api_load_json.h"

int crypto_api_load_json(const char * file_path, json_object ** p_jobj) {
    int rval = 0;
    char * buffer;
    size_t buffer_size;
    struct json_object * jobj;
    enum json_tokener_error jerror;
    struct json_tokener * tokener;
    const char * error_desc;

    if (NULL == file_path || NULL == p_jobj) {
        fprintf(stderr, "ERROR in parse_json_file: INVALID ARGUMENTS!\n");
        rval = -1;
        goto FAILED1;
    }

    if (0 != crypto_api_load_file(file_path, &buffer, &buffer_size)) {
        fprintf(stderr, "ERROR in parse_json_file: crypto_api_load_file() failed!\n");
        rval = -1;
        goto FAILED1;
    }

    tokener = json_tokener_new();
    if (NULL == tokener) {
        fprintf(stderr, "ERROR in parse_json_file: json_tokener_new() failed!\n");
        rval = -1;
        goto FAILED2;
    }

    jobj = json_tokener_parse_ex(tokener, buffer, (int)buffer_size);
    if (NULL == jobj) {
        fprintf(stderr, "ERROR in parse_json_file: json_tokener_parse_ex() failed!\n");
        jerror = json_tokener_get_error(tokener);
        error_desc = json_tokener_error_desc(jerror);
        if (NULL != error_desc) {
            fprintf(stderr, "json_tokener_error_desc: %s\n", error_desc);
        }
        rval = -1;
        goto FAILED3;
    }

    *p_jobj = jobj;

FAILED3:
    json_tokener_free(tokener);

FAILED2:
	memset(buffer, 0, buffer_size);
    free(buffer);

FAILED1:
    return rval;
}
