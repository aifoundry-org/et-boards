#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "crypto_api_config.h"
#include "crypto_api.h"
#include "crypto_api_load_file.h"

int crypto_api_load_file(const char * file_path, char ** buffer, size_t * buffer_size) {
    int rval = 0;
    long offset;
    size_t size;
    FILE * f;
    char * ptr;

    if (NULL == buffer || NULL == buffer_size) {
        fprintf(stderr, "ERROR in load_file: INVALID ARGUMENTS!\n");
        rval = -1;
        goto FAILED1;
    }
    
    f = fopen(file_path, "r");
    if (NULL == f) {
        fprintf(stderr, "ERROR in load_file: fopen(\"%s\", \"r\") failed!\n", file_path);
        rval = -1;
        goto FAILED1;
    }

    if (-1 == fseek(f, 0L, SEEK_END)) {
        fprintf(stderr, "ERROR in load_file: fseek(SEEK_END) failed!\n");
        fclose(f);
        rval = -1;
        goto FAILED2;
    }

    offset = ftell(f);
    if (-1 == offset) {
        fprintf(stderr, "ERROR in load_file: ftell() failed!\n");
        rval = -1;
        goto FAILED2;
    }
    size = (size_t)offset;

    if (-1 == fseek(f, 0L, SEEK_SET)) {
        fprintf(stderr, "ERROR in load_file: fseek(SEEK_SET) failed!\n");
        fclose(f);
        rval = -1;
        goto FAILED2;
    }

    ptr = (char*)malloc(size);
    if (NULL == ptr) {
        fprintf(stderr, "ERROR in load_file: malloc(%lu) failed!\n", size);
        rval = -1;
        goto FAILED2;
    }

    if (size != fread(ptr, 1, size, f)) {
        fprintf(stderr, "ERROR in load_file: fread(%lu) failed!\n", size);
        rval = -1;
        goto FAILED3;
    }

    *buffer_size = size;
    *buffer = ptr;
    fclose(f);
    return 0;

FAILED3:
    free(ptr);

FAILED2:
    fclose(f);

FAILED1:
    return rval;
}

int crypto_api_load_certificate_request(ESPERANTO_CERTIFICATE_REQUEST_t * pcertificate, const char * certificate_file_path) {
    int rval;
    ESPERANTO_CERTIFICATE_REQUEST_t * certificate;
    size_t buffer_size = 0;
    char * buffer = NULL;
    if (0 != crypto_api_load_file(certificate_file_path, &buffer, &buffer_size)) {
        fprintf(stderr, "Error in crypto_api_load_certificate_request: crypto_api_load_file() failed!\n");
        rval = -1;
        goto DONE;
    }

    if (sizeof(ESPERANTO_CERTIFICATE_REQUEST_t) != buffer_size) {
        fprintf(stderr, "Error in crypto_api_load_certificate_request: invalid file size!\n");
        rval = -1;
        goto DONE;
    }

    certificate = (ESPERANTO_CERTIFICATE_REQUEST_t*)buffer;
    if (CURRENT_CERTIFICATE_REQUEST_HEADER_TAG != certificate->request_info.header_tag) {
        fprintf(stderr, "Error in crypto_api_load_certificate_request: invalid file tag!\n");
        rval = -1;
        goto DONE;
    }
    if (certificate->request_info.version_tag > CURRENT_CERTIFICATE_REQUEST_VERSION_TAG) {
        fprintf(stderr, "Error in crypto_api_load_certificate_request: certificate version higher than the tool supported version!\n");
        rval = -1;
        goto DONE;
    }

    *pcertificate = *certificate;
    buffer = NULL;
    rval = 0;

DONE:
    if (NULL != buffer) {
        free(buffer);
    }
    return rval;
}

int crypto_api_load_certificate(ESPERANTO_CERTIFICATE_t * pcertificate, const char * certificate_file_path) {
    int rval;
    ESPERANTO_CERTIFICATE_t * certificate;
    size_t buffer_size = 0;
    char * buffer = NULL;
    if (0 != crypto_api_load_file(certificate_file_path, &buffer, &buffer_size)) {
        fprintf(stderr, "Error in crypto_api_load_certificate: crypto_api_load_file() failed!\n");
        rval = -1;
        goto DONE;
    }

    if (sizeof(ESPERANTO_CERTIFICATE_t) != buffer_size) {
        fprintf(stderr, "Error in crypto_api_load_certificate: invalid file size!\n");
        rval = -1;
        goto DONE;
    }

    certificate = (ESPERANTO_CERTIFICATE_t*)buffer;
    if (CURRENT_CERTIFICATE_HEADER_TAG != certificate->certificate_info.header_tag) {
        fprintf(stderr, "Error in crypto_api_load_certificate: invalid file tag!\n");
        rval = -1;
        goto DONE;
    }
    if (certificate->certificate_info.version_tag > CURRENT_CERTIFICATE_VERSION_TAG) {
        fprintf(stderr, "Error in crypto_api_load_certificate: certificate version higher than the tool supported version!\n");
        rval = -1;
        goto DONE;
    }

    *pcertificate = *certificate;
    buffer = NULL;
    rval = 0;

DONE:
    if (NULL != buffer) {
        free(buffer);
    }
    return rval;
}

int crypto_api_load_signed_elf_file(ESPERANTO_IMAGE_FILE_HEADER_t ** pheader, const uint8_t ** pcode_and_data, uint32_t * pcode_and_data_size, const char * signed_elf_file_path) {
    int rval;
    ESPERANTO_IMAGE_FILE_HEADER_t * header;
    size_t buffer_size = 0;
    char * buffer = NULL;
    if (0 != crypto_api_load_file(signed_elf_file_path, &buffer, &buffer_size)) {
        fprintf(stderr, "Error in crypto_api_load_signed_elf_file: crypto_api_load_file() failed!\n");
        rval = -1;
        goto DONE;
    }

    if (buffer_size < sizeof(ESPERANTO_IMAGE_FILE_HEADER_t)) {
        fprintf(stderr, "Error in crypto_api_load_signed_elf_file: invalid file size!\n");
        rval = -1;
        goto DONE;
    }

    header = (ESPERANTO_IMAGE_FILE_HEADER_t*)buffer;
    if (CURRENT_EXEC_FILE_HEADER_TAG != header->info.file_header_tag) {
        fprintf(stderr, "Error in crypto_api_load_signed_elf_file: invalid file tag!\n");
        rval = -1;
        goto DONE;
    }
    if (header->info.file_version_tag > CURRENT_EXEC_FILE_VERSION_TAG) {
        fprintf(stderr, "Error in crypto_api_load_signed_elf_file: certificate version higher than the tool supported version!\n");
        rval = -1;
        goto DONE;
    }

    *pheader = header;
    *pcode_and_data = (const uint8_t *)(buffer + sizeof(ESPERANTO_IMAGE_FILE_HEADER_t));
    *pcode_and_data_size = (uint32_t)(buffer_size - sizeof(ESPERANTO_IMAGE_FILE_HEADER_t));
    buffer = NULL;
    rval = 0;

DONE:
    if (NULL != buffer) {
        free(buffer);
    }
    return rval;
}

int crypto_api_load_signed_raw_file(ESPERANTO_RAW_IMAGE_FILE_HEADER_t ** pheader, const uint8_t ** praw_data, uint32_t * praw_data_size, const char * signed_raw_file_path) {
    int rval;
    ESPERANTO_RAW_IMAGE_FILE_HEADER_t * header;
    size_t buffer_size = 0;
    char * buffer = NULL;
    if (0 != crypto_api_load_file(signed_raw_file_path, &buffer, &buffer_size)) {
        fprintf(stderr, "Error in crypto_api_load_signed_elf_file: crypto_api_load_file() failed!\n");
        rval = -1;
        goto DONE;
    }

    if (buffer_size < sizeof(ESPERANTO_RAW_IMAGE_FILE_HEADER_t)) {
        fprintf(stderr, "Error in crypto_api_load_signed_elf_file: invalid file size!\n");
        rval = -1;
        goto DONE;
    }

    header = (ESPERANTO_RAW_IMAGE_FILE_HEADER_t*)buffer;
    if (CURRENT_RAW_FILE_HEADER_TAG != header->info.file_header_tag) {
        fprintf(stderr, "Error in crypto_api_load_signed_elf_file: invalid file tag!\n");
        rval = -1;
        goto DONE;
    }
    if (header->info.file_version_tag > CURRENT_RAW_FILE_VERSION_TAG) {
        fprintf(stderr, "Error in crypto_api_load_signed_elf_file: file version higher than the tool supported version!\n");
        rval = -1;
        goto DONE;
    }

    *pheader = header;
    *praw_data = (const uint8_t *)(buffer + sizeof(ESPERANTO_RAW_IMAGE_FILE_HEADER_t));
    *praw_data_size = (uint32_t)(buffer_size - sizeof(ESPERANTO_RAW_IMAGE_FILE_HEADER_t));
    buffer = NULL;
    rval = 0;

DONE:
    if (NULL != buffer) {
        free(buffer);
    }
    return rval;
}
