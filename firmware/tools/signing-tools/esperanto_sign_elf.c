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

#include "esperanto_sign_elf.h"
#include "esperanto_load_elf.h"

// version string
const char *argp_program_version = "esperanto_sign_elf 1.0";

// bug address
const char *argp_program_bug_address = "problems@esperantotech.com";

// short program documentation
static char doc[] = "Creates an Esperanto Signed Executable image";

// arguments description
static char args_doc[] = "<image_type> <signed_image_file> <original_elf_file> <signing_certificate_file> <signing_private_key> <hash_algorithm> <revocation_counter>";

// options
static struct argp_option options[] = {
    { "verbose",        'v', NULL,                      0,                      "Produce verbose output",   0 },
    { "quiet",          'q', NULL,                      0,                      "Don't produce any output", 0 },
    { "silent",         's', 0,                         OPTION_ALIAS,           NULL,                       0 },
    { "image-types",    'i', NULL,                      0,                      "List supported image types", 0 },
    { "enc",            'e', "<encryption_aes_key>",    0,                      "Encrypt the image",        0 },
    { "mac",            'm', "<mac_aes_key>",           0,                      "Sign encrypted image",     0 },
    { "file-version",   'F', "<file_version>",          0,                      "Specify file version",     0 },
    { "git-hash",       'H', "<GIT_hash>",              0,                      "Specify GIT hash",         0 },
    { "git-version",    'G', "<GIT_version>",           0,                      "Specify GIT version",      0 },
    { "no-sign",        'N', NULL,                      0,                      "Do not sign the image",    0 },
    { 0 }
};

static void list_image_types(void) {
    ESPERANTO_IMAGE_TYPE_t image_type;
    
    printf("Supported image types:");
    for (image_type = 1; image_type < ESPERANTO_IMAGE_TYPE_COUNT; image_type++) {
        if (0 == image_type) continue;
        printf(" %s", executable_image_type_to_type_name(image_type));
        if (image_type != (ESPERANTO_IMAGE_TYPE_COUNT - 1)) {
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
    case 'F':
        g_arguments.file_version = arg;
        break;
    case 'H':
        g_arguments.git_hash = arg;
        break;
    case 'G':
        g_arguments.git_version = arg;
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
    ESPERANTO_IMAGE_TYPE_t image_type;
    HASH_ALG_t hash_algorithm;
    uint32_t file_version;
    char git_version[MAX_GIT_VERSION_LENGTH + 1];
    uint32_t revocation_counter;
    char * endptr;
    uint32_t git_hash_string_length, git_version_string_length;
    ESPERANTO_CERTIFICATE_t signing_certificate;
    bool signature_ok;
    FILE * signed_elf_file = NULL;
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

    ESPERANTO_IMAGE_FILE_HEADER_t esperanto_image_header;
    uint8_t * esperanto_image_code_and_data = NULL;
    uint32_t esperanto_image_code_and_data_length;
    uint32_t git_hash_size, code_and_data_hash_size;
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
    g_arguments.file_version = NULL;
    g_arguments.git_hash = NULL;
    g_arguments.git_version = NULL;

    const IMAGE_VERSION_INFO_t * p_image_info = NULL;

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

    image_type = executable_image_type_name_to_type(g_arguments.args[ARGS_IMAGE_TYPE], (uint32_t)strlen(g_arguments.args[ARGS_IMAGE_TYPE]));
    if (ESPERANTO_IMAGE_TYPE_INVALID == image_type) {
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
        printf("  original_elf_file: %s\n", g_arguments.args[ARGS_ORIGINAL_ELF_FILE]);
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

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // try to load and parse the ELF file
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    memset(&esperanto_image_header, 0, sizeof(esperanto_image_header));
    if (0 != load_elf_file(g_arguments.args[ARGS_ORIGINAL_ELF_FILE], &esperanto_image_code_and_data, &esperanto_image_code_and_data_length, &esperanto_image_header, &p_image_info)) {
        fprintf(stderr, "Failed to load the ELF file '%s'", g_arguments.args[ARGS_ORIGINAL_ELF_FILE]);
        rval = -1;
        goto DONE;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // verify the file version, GIT version and GIT hash info
    // - the above data might come from the ELF file or the command line arguments
    // - if it is provided both in the ELF file and command line arguments, make sure it matches
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    if (NULL != p_image_info) {
        if (NULL != g_arguments.file_version) {
            file_version = (uint32_t)strtoul(g_arguments.file_version, &endptr, 0);
            if (0 != *endptr) {
                fprintf(stderr, "ERROR: invalid file_version argument '%s'!\n", g_arguments.file_version);
                rval = -1;
                goto DONE;
            }
            if (file_version != p_image_info->file_version) {
                fprintf(stderr, "ERROR: the file version found in the ELF file (0x%x) does not match the file_version argument '%s'!\n", p_image_info->file_version, g_arguments.file_version);
                rval = -1;
                goto DONE;
            }
        }
        file_version = p_image_info->file_version;

        memcpy(git_version, p_image_info->git_version, MAX_GIT_VERSION_LENGTH);
        git_version[MAX_GIT_VERSION_LENGTH] = 0;
        git_version_string_length = (uint32_t)strlen(git_version);
        if (NULL != g_arguments.git_version) {
            git_version_string_length = (uint32_t)strlen(g_arguments.git_version);
            if (git_version_string_length > sizeof(esperanto_image_header.info.image_info_and_signaure.info.public_info.git_version)) {
                fprintf(stderr, "ERROR: GIT_version argument is too long!  (Maximum supported length: %lu)!\n", sizeof(esperanto_image_header.info.image_info_and_signaure.info.public_info.git_version));
                rval = -1;
                goto DONE;
            }
            if (0 != strcmp(g_arguments.git_version, git_version)) {
                fprintf(stderr, "ERROR: the GIT version found in the ELF file ('%s') does not match the GIT_version argument '%s'!\n", git_version, g_arguments.git_version);
                rval = -1;
                goto DONE;
            }
        }

        memcpy(git_hash, p_image_info->git_hash, sizeof(git_hash));
        git_hash_size = sizeof(git_hash);
        if (NULL != g_arguments.git_hash) {
            memset(git_hash, 0, sizeof(git_hash));
            git_hash_string_length = (uint32_t)strlen(g_arguments.git_hash);
            if (0 != crypto_api_convert_hexblob(g_arguments.git_hash, git_hash_string_length, git_hash, sizeof(git_hash), &git_hash_size)) {
                fprintf(stderr, "ERROR: invalid GIT_hash argument '%s'!\n", g_arguments.git_hash);
                rval = -1;
                goto DONE;
            }
            if (0 != memcmp(git_hash, p_image_info->git_hash, sizeof(git_hash))) {
                fprintf(stderr, "ERROR: the GIT hash found in the ELF file (");
                crypto_api_dump_githash(true, p_image_info->git_hash);
                fprintf(stderr, ") does not match the GIT_hash argument (");
                crypto_api_dump_githash(true, git_hash);
                fprintf(stderr, ")!\n");
                rval = -1;
                goto DONE;
            }
        }
    } else {
        if (NULL == g_arguments.file_version) {
            if (!g_arguments.silent) {
                fprintf(stderr, "File_version information not found in the ELF file!\n");
                fprintf(stderr, "Missing file_version argument!\n");
                rval = -1;
                goto DONE;
            }
        }
        file_version = (uint32_t)strtoul(g_arguments.file_version, &endptr, 0);
        if (0 != *endptr) {
            fprintf(stderr, "ERROR: invalid file_version argument '%s'!\n", g_arguments.file_version);
            rval = -1;
            goto DONE;
        }

        if (NULL == g_arguments.git_version) {
            if (!g_arguments.silent) {
                fprintf(stderr, "GIT_version information not found in the ELF file!\n");
                fprintf(stderr, "Missing GIT_version argument!\n");
                rval = -1;
                goto DONE;
            }
        }
        git_version_string_length = (uint32_t)strlen(g_arguments.git_version);
        if (git_version_string_length > sizeof(esperanto_image_header.info.image_info_and_signaure.info.public_info.git_version)) {
            fprintf(stderr, "ERROR: GIT_version argument is too long!  (Maximum supported length: %lu)!\n", sizeof(esperanto_image_header.info.image_info_and_signaure.info.public_info.git_version));
            rval = -1;
            goto DONE;
        }
        memcpy(git_version, g_arguments.git_version, git_version_string_length);
        git_version[MAX_GIT_VERSION_LENGTH] = 0;

        if (NULL == g_arguments.git_hash) {
            if (!g_arguments.silent) {
                fprintf(stderr, "GIT_hash information not found in the ELF file!\n");
                fprintf(stderr, "Missing GIT_hash argument!\n");
                rval = -1;
                goto DONE;
            }
        }
        git_hash_string_length = (uint32_t)strlen(g_arguments.git_hash);
        if (0 != crypto_api_convert_hexblob(g_arguments.git_hash, git_hash_string_length, git_hash, sizeof(git_hash), &git_hash_size)) {
            fprintf(stderr, "ERROR: invalid GIT_hash argument '%s'!\n", g_arguments.git_hash);
            rval = -1;
            goto DONE;
        }
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
    // compute code & data hash
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    if (0 != crypto_api_sha_hash(hash_algorithm, 
				 esperanto_image_header.info.image_info_and_signaure.info.public_info.code_and_data_hash, 
				 sizeof(esperanto_image_header.info.image_info_and_signaure.info.public_info.code_and_data_hash), 
				 &code_and_data_hash_size, 
				 esperanto_image_code_and_data, 
				 esperanto_image_code_and_data_length)) {
	fprintf(stderr, "Failed to compute the code+data hash!\n");
	rval = -1;
	goto DONE;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // fill in the rest of the image header data
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    esperanto_image_header.info.file_header_tag = CURRENT_EXEC_FILE_HEADER_TAG;
    esperanto_image_header.info.file_version_tag = CURRENT_EXEC_FILE_VERSION_TAG;
    esperanto_image_header.info.image_info_and_signaure.info.public_info.header_tag = CURRENT_EXEC_IMAGE_HEADER_TAG;
    esperanto_image_header.info.image_info_and_signaure.info.public_info.version_tag = CURRENT_EXEC_IMAGE_VERSION_TAG;
    memcpy(esperanto_image_header.info.image_info_and_signaure.info.public_info.git_hash, git_hash, git_hash_size);
    memcpy(esperanto_image_header.info.image_info_and_signaure.info.public_info.git_version, git_version, git_version_string_length);
    esperanto_image_header.info.image_info_and_signaure.info.public_info.image_type = image_type;
    esperanto_image_header.info.image_info_and_signaure.info.public_info.file_version = file_version;
    esperanto_image_header.info.image_info_and_signaure.info.public_info.revocation_counter = revocation_counter;
    esperanto_image_header.info.image_info_and_signaure.info.public_info.code_and_data_hash_algorithm = hash_algorithm;
    esperanto_image_header.info.image_info_and_signaure.info.public_info.code_and_data_size = esperanto_image_code_and_data_length;
    if (0 != crypto_api_get_current_date_and_time(&esperanto_image_header.info.image_info_and_signaure.info.public_info.fileDateAndTimeStamp)) {
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
	esperanto_image_header.info.signing_certificate = signing_certificate;


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

        if (0 != crypto_api_pk_sign(&esperanto_image_header.info.image_info_and_signaure.info_signature, &esperanto_image_header.info.image_info_and_signaure.info, sizeof(esperanto_image_header.info.image_info_and_signaure.info), hash_algorithm, private_key_object_handle)) {
            fprintf(stderr, "Private key signature problem: crypto_api_pk_sign() failed!\n");
            rval = -1;
            goto DONE;
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // verify the signature using the certificate public key
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        if (0 != crypto_api_verify_with_public_key(&esperanto_image_header.info.image_info_and_signaure.info_signature, &esperanto_image_header.info.image_info_and_signaure.info, sizeof(esperanto_image_header.info.image_info_and_signaure.info), &signing_certificate.certificate_info.subject_public_key, &signature_ok)) {
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
    // print image data before optional encryption
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    if (0 != crypto_view_signed_elf_header(&esperanto_image_header, g_arguments.silent, g_arguments.verbose, true, true, false, g_arguments.do_not_sign)) {
        fprintf(stderr, "crypto_view_signed_elf_header() failed!\n");
        rval = -1;
        goto DONE;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // encrypt signed ELF image
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
        if (0 != crypto_api_random(enc_secret_key_session_handle, esperanto_image_header.info.encryption_IV, sizeof(esperanto_image_header.info.encryption_IV))) {
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

    if (0 != crypto_view_signed_elf_header(&esperanto_image_header, g_arguments.silent, g_arguments.verbose, false, false, true, g_arguments.do_not_sign)) {
        fprintf(stderr, "crypto_view_signed_elf_header() failed!\n");
        rval = -1;
        goto DONE;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // save signed ELF image
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    signed_elf_file = fopen(g_arguments.args[ARGS_SIGNED_IMAGE_FILE], "wb");
    if (NULL == signed_elf_file) {
        fprintf(stderr, "Could not open signed image file '%s' for writing!\n", g_arguments.args[ARGS_SIGNED_IMAGE_FILE]);
        rval = -1;
        goto DONE;
    }

    if (1 != fwrite(&esperanto_image_header, sizeof(esperanto_image_header), 1, signed_elf_file)) {
        fprintf(stderr, "Failed to write the signed image header!\n");
        rval = -1;
        goto DONE;
    }

    if (1 != fwrite(esperanto_image_code_and_data, esperanto_image_code_and_data_length, 1, signed_elf_file)) {
        fprintf(stderr, "Failed to write the signed image code+data!\n");
        rval = -1;
        goto DONE;
    }

    if (!g_arguments.silent) {
        printf("Successfuly saved the signed image to file '%s'.\n", g_arguments.args[ARGS_SIGNED_IMAGE_FILE]);
    }

    fclose(signed_elf_file);
    signed_elf_file = NULL;

    rval = 0;

DONE:

    if (NULL != signed_elf_file) {
        fclose(signed_elf_file);
    }

    if (NULL != esperanto_image_code_and_data) {
        free(esperanto_image_code_and_data);
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
