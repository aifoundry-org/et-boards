#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <argp.h>
#include <stddef.h>

#include "elf.h"

#include "crypto_api.h"
#include "crypto_api_load_file.h"
#include "crypto_api_load_json.h"
#include "crypto_api_parse_json.h"
#include "crypto_api_view_data.h"

#include "esperanto_sign_raw.h"

// version string
const char *argp_program_version = "esperanto_sign_raw 1.0";

// bug address
const char *argp_program_bug_address = "problems@esperantotech.com";

// short program documentation
static char doc[] = "Creates an Esperanto Signed Raw image";

// arguments description
static char args_doc[] = "<image_type> <signed_image_file> <original_raw_file> <signing_certificate_file> <signing_private_key> <hash_algorithm> <revocation_counter> <file_version> <git_version> <git_hash>";

// options
static struct argp_option options[] = {
    { "verbose",        'v', NULL,                      0,                      "Produce verbose output",   0 },
    { "quiet",          'q', NULL,                      0,                      "Don't produce any output", 0 },
    { "silent",         's', 0,                         OPTION_ALIAS,           NULL,                       0 },
    { "image-types",    'i', NULL,                      0,                      "List supported image types", 0 },
    { "enc",            'e', "<encryption_aes_key>",    0,                      "Encrypt the image",        0 },
    { "mac",            'm', "<mac_aes_key>",           0,                      "Sign encrypted image",     0 },
    { "no-sign",        'N', NULL,                      0,                      "Do not sign the image",    0 },
    { 0 }
};

static void list_image_types(void) {
    ESPERANTO_RAW_IMAGE_TYPE_t image_type;

    printf("Supported image types:");
    for (image_type = 1; image_type < ESPERANTO_RAW_IMAGE_TYPE_COUNT; image_type++) {
        if (0 == image_type) continue;
        printf(" %s", raw_image_type_to_type_name(image_type));
        if (image_type != (ESPERANTO_RAW_IMAGE_TYPE_COUNT - 1)) {
            printf(",");
        }
    }
    printf("\n");
}

// options parser
static error_t parse_opt(int key, char * arg, struct argp_state * state) {
    ARGUMENTS_t * arguments = state->input;

    switch (key) {
    case 'q':
    case 's':
        arguments->silent = true;
        break;
    case 'v':
        arguments->verbose = true;
        break;
    case 'N':
        arguments->do_not_sign = true;
        break;
    case 'i':
        list_image_types();
        exit(-1);

    case 'e':
        arguments->enc_key = arg;
        break;
    case 'm':
        arguments->mac_key = arg;
        break;

    case ARGP_KEY_ARG:
        if (state->arg_num >= ARGS_TOTAL_COUNT) {
            // too many arguments
            argp_usage(state);
        }

        arguments->args[state->arg_num] = arg;
        break;

    case ARGP_KEY_END:
        if (state->arg_num < ARGS_TOTAL_COUNT) {
            // not enough arguments
            argp_usage(state);
        }
        break;

    default:
        return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

// argp data
static struct argp argp = { options, parse_opt, args_doc, doc, NULL, NULL, NULL };

// arguments
ARGUMENTS_t g_arguments;

int main(int argc, char ** argv) {
    int rval;
    ESPERANTO_RAW_IMAGE_TYPE_t raw_image_type;
    HASH_ALG_t hash_algorithm;
    uint32_t file_version;
    char git_version[MAX_GIT_VERSION_LENGTH + 1];
    uint32_t revocation_counter;
    char * endptr;
    uint32_t git_hash_string_length, git_version_string_length;
    ESPERANTO_CERTIFICATE_t signing_certificate;
    bool signature_ok;
    FILE * signed_raw_file = NULL;
    union {
        uint32_t u32;
        struct {
            uint8_t revision;
            uint8_t minor;
            uint8_t major;
            uint8_t reserved;
        };
    } fv;
    bool crypto_api_initialized = false;

    ESPERANTO_RAW_IMAGE_FILE_HEADER_t esperanto_raw_image_header;
    uint8_t * esperanto_raw_image = NULL;
    size_t esperanto_raw_image_length, esperanto_raw_image_file_length;
    uint32_t git_hash_size, raw_image_hash_size;
    uint8_t git_hash[32];

    json_object *         private_key_jobj;
    PKCS_MODULE_INFO_t *  private_key_module_info = NULL;
    PKCS_SESSION_INFO_t * private_key_session_info = NULL;
    PKCS_OBJECT_INFO_t *  private_key_object_info = NULL;
    MODULE_HANDLE_t       private_key_module_handle = NULL;
    SESSION_HANDLE_t      private_key_session_handle = NULL;
    OBJECT_HANDLE_t      private_key_object_handle = NULL;

    json_object *         enc_secret_key_jobj;
    PKCS_MODULE_INFO_t *  enc_secret_key_module_info = NULL;
    PKCS_SESSION_INFO_t * enc_secret_key_session_info = NULL;
    PKCS_OBJECT_INFO_t *  enc_secret_key_object_info = NULL;
    MODULE_HANDLE_t       enc_secret_key_module_handle = NULL;
    SESSION_HANDLE_t      enc_secret_key_session_handle = NULL;
    OBJECT_HANDLE_t       enc_secret_key_object_handle = NULL;

    json_object *         mac_secret_key_jobj;
    PKCS_MODULE_INFO_t *  mac_secret_key_module_info = NULL;
    PKCS_SESSION_INFO_t * mac_secret_key_session_info = NULL;
    PKCS_OBJECT_INFO_t *  mac_secret_key_object_info = NULL;
    MODULE_HANDLE_t       mac_secret_key_module_handle = NULL;
    SESSION_HANDLE_t      mac_secret_key_session_handle = NULL;
    OBJECT_HANDLE_t       mac_secret_key_object_handle = NULL;

    g_arguments.silent = false;
    g_arguments.verbose = false;
    g_arguments.do_not_sign = false;
    g_arguments.enc_key = NULL;
    g_arguments.mac_key = NULL;

    argp_parse(&argp, argc, argv, 0, 0, &g_arguments);

    if (NULL != g_arguments.mac_key && NULL == g_arguments.enc_key) {
        fprintf(stderr, "ERROR: missing encryption key! (option -m also requires option -e)\n");
        rval = -1;
        goto DONE;
    }

    if (NULL != g_arguments.enc_key && NULL == g_arguments.mac_key) {
        fprintf(stderr, "ERROR: missing mac key! (option -e also requires option -m)\n");
        rval = -1;
        goto DONE;
    }

    raw_image_type = raw_image_type_name_to_type(g_arguments.args[ARGS_IMAGE_TYPE], (uint32_t)strlen(g_arguments.args[ARGS_IMAGE_TYPE]));
    if (ESPERANTO_RAW_IMAGE_TYPE_INVALID == raw_image_type) {
        fprintf(stderr, "ERROR: invalid image_type argument '%s'!\n", g_arguments.args[ARGS_IMAGE_TYPE]);
        rval = -1;
        goto DONE;
    }

    hash_algorithm = hash_algorithm_name_to_id(g_arguments.args[ARGS_SIGNING_HASH_ALGORITHM], (uint32_t)strlen(g_arguments.args[ARGS_SIGNING_HASH_ALGORITHM]));
    if (HASH_ALG_INVALID == hash_algorithm) {
        fprintf(stderr, "ERROR: invalid hash_type argument '%s'!\n", g_arguments.args[ARGS_SIGNING_HASH_ALGORITHM]);
        rval = -1;
        goto DONE;
    }

    revocation_counter = (uint32_t)strtoul(g_arguments.args[ARGS_REVOCATION_COUNTER], &endptr, 0);
    if (0 != *endptr) {
        fprintf(stderr, "ERROR: invalid revocation_counter argument '%s'!\n", g_arguments.args[ARGS_REVOCATION_COUNTER]);
        rval = -1;
        goto DONE;
    }

    if (g_arguments.verbose) {
        printf("Arguments:\n");
        printf("  image_type: %s\n", g_arguments.args[ARGS_IMAGE_TYPE]);
        printf("  signe_image_file: %s\n", g_arguments.args[ARGS_SIGNED_IMAGE_FILE]);
        printf("  original_raw_file: %s\n", g_arguments.args[ARGS_ORIGINAL_RAW_FILE]);
        printf("  signing_certificate_file: %s\n", g_arguments.args[ARGS_SIGNING_CERTIFICATE_FILE]);
        printf("  signing_private_key: %s\n", g_arguments.args[ARGS_SIGNING_PRIVATE_KEY_FILE]);
        printf("  signing_hash_algorithm: %s\n", g_arguments.args[ARGS_SIGNING_HASH_ALGORITHM]);
        printf("  revocation_counter: %u\n", revocation_counter);
        if (NULL != g_arguments.enc_key) {
            printf("  Encryption key: %s\n", g_arguments.enc_key);
        }
        if (NULL != g_arguments.mac_key) {
            printf("  MAC key: %s\n", g_arguments.mac_key);
        }
    }

    memset(&esperanto_raw_image_header, 0, sizeof(esperanto_raw_image_header));

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // try to load the raw file
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    if (0 != crypto_api_load_file(g_arguments.args[ARGS_ORIGINAL_RAW_FILE], (char **)&esperanto_raw_image, &esperanto_raw_image_length)) {
        fprintf(stderr, "Failed to load the raw file '%s'", g_arguments.args[ARGS_ORIGINAL_RAW_FILE]);
        rval = -1;
        goto DONE;
    }

    // test if the image is a RISC-V executable ELF file
    if (esperanto_raw_image_length > 0x14) {
        if (ELFMAG0 == esperanto_raw_image[EI_MAG0] &&
            ELFMAG1 == esperanto_raw_image[EI_MAG1] &&
            ELFMAG2 == esperanto_raw_image[EI_MAG2] &&
            ELFMAG3 == esperanto_raw_image[EI_MAG3] &&
            ELFDATA2LSB == esperanto_raw_image[EI_DATA] &&
            EV_CURRENT == esperanto_raw_image[EI_VERSION]) {
            if (ELFCLASS32 == esperanto_raw_image[EI_CLASS]) {
                const Elf32_Ehdr * elf_hdr = (const Elf32_Ehdr*)esperanto_raw_image;
                if (ET_EXEC == elf_hdr->e_type &&
                    EM_RISCV == elf_hdr->e_machine &&
                    EV_CURRENT == elf_hdr->e_version) {
                    fprintf(stderr, "The file '%s' content appears to be a 32-bit RISC-V executable image!  You should use esperanto_sign_elf instead!\n", g_arguments.args[ARGS_ORIGINAL_RAW_FILE]);
                    rval = -1;
                    goto DONE;
                }
            } else if (ELFCLASS64 == esperanto_raw_image[EI_CLASS]) {
                const Elf64_Ehdr * elf_hdr = (const Elf64_Ehdr*)esperanto_raw_image;
                if (ET_EXEC == elf_hdr->e_type &&
                    EM_RISCV == elf_hdr->e_machine &&
                    EV_CURRENT == elf_hdr->e_version) {
                    fprintf(stderr, "The file '%s' content appears to be a 64-bit RISC-V executable image!  You should use esperanto_sign_elf instead!\n", g_arguments.args[ARGS_ORIGINAL_RAW_FILE]);
                    rval = -1;
                    goto DONE;
                }
            }
        }
    }

    // resize the raw data  buffer so that its size is a multiple of 128 bytes
    esperanto_raw_image_file_length = (esperanto_raw_image_length + 127) & 0xFFFFFFFFFFFFFF80;
    if (esperanto_raw_image_file_length > esperanto_raw_image_length) {
        uint8_t * esperanto_resized_image = (uint8_t*)realloc(esperanto_raw_image, esperanto_raw_image_file_length);
        if (NULL == esperanto_resized_image) {
            fprintf(stderr, "Failed to resize the raw data buffer!\n");
            rval = -1;
            goto DONE;
        }
        esperanto_raw_image = esperanto_resized_image;
        memset(esperanto_raw_image + esperanto_raw_image_length, 0, esperanto_raw_image_file_length - esperanto_raw_image_length);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // verify the file version, GIT version and GIT hash info
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    file_version = (uint32_t)strtoul(g_arguments.args[ARGS_FILE_VERSION], &endptr, 0);
    if (0 != *endptr) {
        fprintf(stderr, "ERROR: invalid file_version argument '%s'!\n", g_arguments.args[ARGS_FILE_VERSION]);
        rval = -1;
        goto DONE;
    }

    git_version_string_length = (uint32_t)strlen(g_arguments.args[ARGS_GIT_VERSION]);
    if (git_version_string_length > sizeof(esperanto_raw_image_header.info.image_info_and_signaure.info.git_version)) {
        fprintf(stderr, "ERROR: GIT_version argument is too long!  (Maximum supported length: %lu)!\n", sizeof(esperanto_raw_image_header.info.image_info_and_signaure.info.git_version));
        rval = -1;
        goto DONE;
    }
    memcpy(git_version, g_arguments.args[ARGS_GIT_VERSION], git_version_string_length);
    git_version[MAX_GIT_VERSION_LENGTH] = 0;

    git_hash_string_length = (uint32_t)strlen(g_arguments.args[ARGS_GIT_HASH]);
    if (0 != crypto_api_convert_hexblob(g_arguments.args[ARGS_GIT_HASH], git_hash_string_length, git_hash, sizeof(git_hash), &git_hash_size)) {
        fprintf(stderr, "ERROR: invalid git_hash argument '%s'!\n", g_arguments.args[ARGS_GIT_HASH]);
        rval = -1;
        goto DONE;
    }

    if (g_arguments.verbose) {
        fv.u32 = file_version;
        printf("  file_version: %u.%u.%u (0x%08x)\n", fv.major, fv.minor, fv.revision, file_version);
        printf("  GIT_hash: ");
        crypto_api_dump_githash(false, git_hash);
        printf("\n");
        printf("  GIT_version: %s\n", git_version);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // compute data hash
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    if (0 != crypto_api_sha_hash(hash_algorithm,
                                 esperanto_raw_image_header.info.image_info_and_signaure.info.raw_image_hash,
                                 sizeof(esperanto_raw_image_header.info.image_info_and_signaure.info.raw_image_hash),
                                 &raw_image_hash_size,
                                 esperanto_raw_image,
                                 esperanto_raw_image_file_length)) {
        fprintf(stderr, "Failed to compute the raw image hash!\n");
        rval = -1;
        goto DONE;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // fill in the rest of the image header data
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    esperanto_raw_image_header.info.file_header_tag = CURRENT_RAW_FILE_HEADER_TAG;
    esperanto_raw_image_header.info.file_version_tag = CURRENT_RAW_FILE_VERSION_TAG;
    esperanto_raw_image_header.info.image_info_and_signaure.info.header_tag = CURRENT_RAW_IMAGE_HEADER_TAG;
    esperanto_raw_image_header.info.image_info_and_signaure.info.version_tag = CURRENT_RAW_IMAGE_VERSION_TAG;
    memcpy(esperanto_raw_image_header.info.image_info_and_signaure.info.git_hash, git_hash, git_hash_size);
    memcpy(esperanto_raw_image_header.info.image_info_and_signaure.info.git_version, git_version, git_version_string_length);
    esperanto_raw_image_header.info.image_info_and_signaure.info.image_type = raw_image_type;
    esperanto_raw_image_header.info.image_info_and_signaure.info.file_version = file_version;
    esperanto_raw_image_header.info.image_info_and_signaure.info.revocation_counter = revocation_counter;
    esperanto_raw_image_header.info.image_info_and_signaure.info.raw_image_hash_algorithm = hash_algorithm;
    esperanto_raw_image_header.info.image_info_and_signaure.info.raw_image_size = (uint32_t)esperanto_raw_image_length;
    if (0 != crypto_api_get_current_date_and_time(&esperanto_raw_image_header.info.image_info_and_signaure.info.fileDateAndTimeStamp)) {
        fprintf(stderr, "Failed to get the date & time stamp!\n");
        rval = -1;
        goto DONE;
    }

    if (0 != crypto_api_init()) {
        fprintf(stderr, "crypto_api_init() failed!\n");
        rval = -1;
        goto DONE;
    }
    crypto_api_initialized = true;
    
    if (g_arguments.do_not_sign) {
        if (!g_arguments.silent) {
            printf("WARNING! IMAGE SIGNING DISABLED!\n");
        }
    } else {
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// try to load the signing certificate
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	if (0 != crypto_api_load_certificate(&signing_certificate, g_arguments.args[ARGS_SIGNING_CERTIFICATE_FILE])) {
	    fprintf(stderr, "Failed to load the certificate file '%s'", g_arguments.args[ARGS_SIGNING_CERTIFICATE_FILE]);
	    rval = -1;
	    goto DONE;
	}
	esperanto_raw_image_header.info.signing_certificate = signing_certificate;
	
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // try to load the signing private key info
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        rval = crypto_api_load_json(g_arguments.args[ARGS_SIGNING_PRIVATE_KEY_FILE], &private_key_jobj);
        if (0 != rval) {
            fprintf(stderr, "Failed to load private key!  crypto_api_load_json() failed with error code %d!\n", rval);
            goto DONE;
        }

        rval = crypto_api_json_get_module_info(private_key_jobj, &private_key_module_info);
        if (0 != rval) {
            fprintf(stderr, "Failed to load private key!  crypto_api_json_get_module_info() failed with error code %d!\n", rval);
            goto DONE;
        }

        if (g_arguments.verbose) {
            printf("Private key module:\n");
            crypto_api_dump_pkcs_module_info(private_key_module_info);
        }

        rval = crypto_api_json_get_session_info(private_key_jobj, &private_key_session_info);
        if (0 != rval) {
            fprintf(stderr, "Failed to load private key!  crypto_api_json_get_session_info() failed with error code %d!\n", rval);
            goto DONE;
        }

        if (g_arguments.verbose) {
            printf("Private key session:\n");
            crypto_api_dump_pkcs_session_info(private_key_session_info);
        }

        rval = crypto_api_json_get_object_info(private_key_jobj, &private_key_object_info);
        if (0 != rval) {
            fprintf(stderr, "Failed to load private key!  crypto_api_json_get_object_info() failed with error code %d!\n", rval);
            goto DONE;
        }

        if (g_arguments.verbose) {
            printf("Private key object:\n");
            crypto_api_dump_pkcs_object_info(private_key_object_info);
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // try to open the signing private key
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        if (0 != crypto_api_module_init(&private_key_module_handle, private_key_module_info->module, NULL)) {
            fprintf(stderr, "Private key problem: crypto_api_module_init() failed!\n");
            rval = -1;
            goto DONE;
        }
        if (0 != crypto_api_session_open(&private_key_session_handle, private_key_module_handle, private_key_session_info->flags, private_key_session_info->slot_id, private_key_session_info->users, private_key_session_info->users_count)) {
            fprintf(stderr, "Private key problem: crypto_api_session_open() failed!\n");
            rval = -1;
            goto DONE;
        }
        if (0 != crypto_api_open_private_key(&private_key_object_handle, private_key_session_handle, private_key_object_info)) {
            fprintf(stderr, "Private key problem: crypto_api_open_private_key() failed!\n");
            rval = -1;
            goto DONE;
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // try to sign the header using the private key
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        if (0 != crypto_api_pk_sign(&esperanto_raw_image_header.info.image_info_and_signaure.info_signature, &esperanto_raw_image_header.info.image_info_and_signaure.info, sizeof(esperanto_raw_image_header.info.image_info_and_signaure.info), hash_algorithm, private_key_object_handle)) {
            fprintf(stderr, "Private key signature problem: crypto_api_pk_sign() failed!\n");
            rval = -1;
            goto DONE;
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // verify the signature using the certificate public key
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        if (0 != crypto_api_verify_with_public_key(&esperanto_raw_image_header.info.image_info_and_signaure.info_signature, &esperanto_raw_image_header.info.image_info_and_signaure.info, sizeof(esperanto_raw_image_header.info.image_info_and_signaure.info), &signing_certificate.certificate_info.subject_public_key, &signature_ok)) {
            fprintf(stderr, "Private key signature problem: crypto_api_verify_with_public_key() failed!\n");
            rval = -1;
            goto DONE;
        }
        if (!signature_ok) {
            fprintf(stderr, "Private key signature problem: signature verification using certificate failed!\n");
            rval = -1;
            goto DONE;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // encrypt signed raw image
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    if (NULL != g_arguments.enc_key) {
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // load the encryption key info

        rval = crypto_api_load_json(g_arguments.enc_key, &enc_secret_key_jobj);
        if (0 != rval) {
            fprintf(stderr, "Failed to load encryption key!  crypto_api_load_json() failed with error code %d!\n", rval);
            goto DONE;
        }

        rval = crypto_api_json_get_module_info(enc_secret_key_jobj, &enc_secret_key_module_info);
        if (0 != rval) {
            fprintf(stderr, "Failed to load encryption key!  crypto_api_json_get_module_info() failed with error code %d!\n", rval);
            goto DONE;
        }

        if (g_arguments.verbose) {
            printf("Encryption key module:\n");
            crypto_api_dump_pkcs_module_info(enc_secret_key_module_info);
        }

        rval = crypto_api_json_get_session_info(enc_secret_key_jobj, &enc_secret_key_session_info);
        if (0 != rval) {
            fprintf(stderr, "Failed to load encryption key!  crypto_api_json_get_session_info() failed with error code %d!\n", rval);
            goto DONE;
        }

        if (g_arguments.verbose) {
            printf("Encryption key session:\n");
            crypto_api_dump_pkcs_session_info(enc_secret_key_session_info);
        }

        rval = crypto_api_json_get_object_info(enc_secret_key_jobj, &enc_secret_key_object_info);
        if (0 != rval) {
            fprintf(stderr, "Failed to load encryption key!  crypto_api_json_get_object_info() failed with error code %d!\n", rval);
            goto DONE;
        }

        if (g_arguments.verbose) {
            printf("Encryption secret key object:\n");
            crypto_api_dump_pkcs_object_info(enc_secret_key_object_info);
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // open the encryption key

        if (0 != crypto_api_module_init(&enc_secret_key_module_handle, enc_secret_key_module_info->module, NULL)) {
            fprintf(stderr, "Encryption key problem: crypto_api_module_init() failed!\n");
            rval = -1;
            goto DONE;
        }
        if (0 != crypto_api_session_open(&enc_secret_key_session_handle, enc_secret_key_module_handle, enc_secret_key_session_info->flags, enc_secret_key_session_info->slot_id, enc_secret_key_session_info->users, enc_secret_key_session_info->users_count)) {
            fprintf(stderr, "Encryption key problem: crypto_api_session_open() failed!\n");
            rval = -1;
            goto DONE;
        }
        if (0 != crypto_api_open_secret_key(&enc_secret_key_object_handle, enc_secret_key_session_handle, enc_secret_key_object_info)) {
            fprintf(stderr, "Encryption key problem: crypto_api_open_private_key() failed!\n");
            rval = -1;
            goto DONE;
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // load the MAC key info

        rval = crypto_api_load_json(g_arguments.mac_key, &mac_secret_key_jobj);
        if (0 != rval) {
            fprintf(stderr, "Failed to load MAC key!  crypto_api_load_json() failed with error code %d!\n", rval);
            goto DONE;
        }

        rval = crypto_api_json_get_module_info(mac_secret_key_jobj, &mac_secret_key_module_info);
        if (0 != rval) {
            fprintf(stderr, "Failed to load MAC key!  crypto_api_json_get_module_info() failed with error code %d!\n", rval);
            goto DONE;
        }

        if (g_arguments.verbose) {
            printf("MAC key module:\n");
            crypto_api_dump_pkcs_module_info(mac_secret_key_module_info);
        }

        rval = crypto_api_json_get_session_info(mac_secret_key_jobj, &mac_secret_key_session_info);
        if (0 != rval) {
            fprintf(stderr, "Failed to load MAC key!  crypto_api_json_get_session_info() failed with error code %d!\n", rval);
            goto DONE;
        }

        if (g_arguments.verbose) {
            printf("MAC key session:\n");
            crypto_api_dump_pkcs_session_info(mac_secret_key_session_info);
        }

        rval = crypto_api_json_get_object_info(mac_secret_key_jobj, &mac_secret_key_object_info);
        if (0 != rval) {
            fprintf(stderr, "Failed to load MAC key!  crypto_api_json_get_object_info() failed with error code %d!\n", rval);
            goto DONE;
        }

        if (g_arguments.verbose) {
            printf("MAC secret key object:\n");
            crypto_api_dump_pkcs_object_info(mac_secret_key_object_info);
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // open the MAC key

        if (0 != crypto_api_module_init(&mac_secret_key_module_handle, mac_secret_key_module_info->module, NULL)) {
            fprintf(stderr, "MAC key problem: crypto_api_module_init() failed!\n");
            rval = -1;
            goto DONE;
        }
        if (0 != crypto_api_session_open(&mac_secret_key_session_handle, mac_secret_key_module_handle, mac_secret_key_session_info->flags, mac_secret_key_session_info->slot_id, mac_secret_key_session_info->users, mac_secret_key_session_info->users_count)) {
            fprintf(stderr, "MAC key problem: crypto_api_session_open() failed!\n");
            rval = -1;
            goto DONE;
        }
        if (0 != crypto_api_open_secret_key(&mac_secret_key_object_handle, mac_secret_key_session_handle, mac_secret_key_object_info)) {
            fprintf(stderr, "MAC key problem: crypto_api_open_private_key() failed!\n");
            rval = -1;
            goto DONE;
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // generate the IV
        if (0 != crypto_api_random(enc_secret_key_session_handle, esperanto_raw_image_header.info.encryption_IV, sizeof(esperanto_raw_image_header.info.encryption_IV))) {
            fprintf(stderr, "Encryption problem: crypto_api_random() failed to generate IV!\n");
            rval = -1;
            goto DONE;
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // encrypt the image

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // encrypt the image

    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // print file header after optional encryption
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    if (0 != crypto_view_signed_raw_header(&esperanto_raw_image_header, g_arguments.silent, g_arguments.verbose, true, true, g_arguments.do_not_sign)) {
        fprintf(stderr, "crypto_view_signed_raw_header() failed!\n");
        rval = -1;
        goto DONE;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // save signed raw image
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    signed_raw_file = fopen(g_arguments.args[ARGS_SIGNED_IMAGE_FILE], "wb");
    if (NULL == signed_raw_file) {
        fprintf(stderr, "Could not open signed image file '%s' for writing!\n", g_arguments.args[ARGS_SIGNED_IMAGE_FILE]);
        rval = -1;
        goto DONE;
    }

    if (1 != fwrite(&esperanto_raw_image_header, sizeof(esperanto_raw_image_header), 1, signed_raw_file)) {
        fprintf(stderr, "Failed to write the signed image header!\n");
        rval = -1;
        goto DONE;
    }

    if (1 != fwrite(esperanto_raw_image, esperanto_raw_image_file_length, 1, signed_raw_file)) {
        fprintf(stderr, "Failed to write the signed raw image data!\n");
        rval = -1;
        goto DONE;
    }

    if (!g_arguments.silent) {
        printf("Successfuly saved the signed raw image to file '%s'.\n", g_arguments.args[ARGS_SIGNED_IMAGE_FILE]);
    }

    fclose(signed_raw_file);
    signed_raw_file = NULL;

    rval = 0;

DONE:

    if (NULL != signed_raw_file) {
        fclose(signed_raw_file);
    }

    if (NULL != esperanto_raw_image) {
        free(esperanto_raw_image);
    }

    if (NULL != private_key_object_handle) {
        if (0 != crypto_api_close_private_key(private_key_object_handle)) {
            fprintf(stderr, "crypto_api_close_private_key() failed!\n");
        }
    }
    if (NULL != private_key_session_handle) {
        if (0 != crypto_api_session_close(private_key_session_handle)) {
            fprintf(stderr, "crypto_api_session_close() failed!\n");
        }
    }
    if (NULL != private_key_module_handle) {
        if (0 != crypto_api_module_cleanup(private_key_module_handle)) {
            fprintf(stderr, "crypto_api_module_cleanup() failed!\n");
        }
    }
    if (crypto_api_initialized && !g_arguments.do_not_sign) {
        crypto_api_free_object_info(private_key_object_info);
        crypto_api_free_session_info(private_key_session_info);
        crypto_api_free_module_info(private_key_module_info);
    }

    if (NULL != enc_secret_key_object_handle) {
        if (0 != crypto_api_close_secret_key(enc_secret_key_object_handle)) {
            fprintf(stderr, "crypto_api_close_secret_key() failed!\n");
        }
    }
    if (NULL != enc_secret_key_session_handle) {
        if (0 != crypto_api_session_close(enc_secret_key_session_handle)) {
            fprintf(stderr, "crypto_api_session_close() failed!\n");
        }
    }
    if (NULL != enc_secret_key_module_handle) {
        if (0 != crypto_api_module_cleanup(enc_secret_key_module_handle)) {
            fprintf(stderr, "crypto_api_module_cleanup() failed!\n");
        }
    }
    if (crypto_api_initialized) {
        crypto_api_free_object_info(enc_secret_key_object_info);
        crypto_api_free_session_info(enc_secret_key_session_info);
        crypto_api_free_module_info(enc_secret_key_module_info);
    }

    if (NULL != mac_secret_key_object_handle) {
        if (0 != crypto_api_close_secret_key(mac_secret_key_object_handle)) {
            fprintf(stderr, "crypto_api_close_secret_key() failed!\n");
        }
    }
    if (NULL != mac_secret_key_session_handle) {
        if (0 != crypto_api_session_close(mac_secret_key_session_handle)) {
            fprintf(stderr, "crypto_api_session_close() failed!\n");
        }
    }
    if (NULL != mac_secret_key_module_handle) {
        if (0 != crypto_api_module_cleanup(mac_secret_key_module_handle)) {
            fprintf(stderr, "crypto_api_module_cleanup() failed!\n");
        }
    }

    if (crypto_api_initialized) {
        crypto_api_free_object_info(mac_secret_key_object_info);
        crypto_api_free_session_info(mac_secret_key_session_info);
        crypto_api_free_module_info(mac_secret_key_module_info);

        if (0 != crypto_api_cleanup()) {
            fprintf(stderr, "crypto_api_cleanup() failed!\n");
        }
    }

    return rval;
}
