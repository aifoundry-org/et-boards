#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <json-c/json.h>

#include "crypto_api.h"
#include "crypto_api_view_data.h"

static void dumphex(const void * data, uint32_t data_size, uint32_t max_line_length) {
    uint32_t col = 0;
    const uint8_t * ps = (const uint8_t *)data;
    const uint8_t * pe = ps + data_size;
    while (ps < pe) {
        printf(" %02x", *ps);
        ps++;
        col++;
        if (col == max_line_length) {
            printf("\n");
            col = 0;
        }
    }
    if (0 != col && 0 != max_line_length) {
        printf("\n");
    }
}

void crypto_api_dump_githash(bool ferror, const uint8_t hash[256/8]) {
    uint32_t n;
    FILE * stream;

    if (ferror) {
        stream = stderr;
    } else {
        stream = stdout;
    }

    for (n = 0; n < (160/8); n++) {
        fprintf(stream, "%02x", hash[n]);
    }

    fprintf(stream, " ");
    
    for (; n < (256/8); n++) {
        fprintf(stream, "%02x", hash[n]);
    }
}

static void print_vstr(const char * str, uint8_t len) {
    while (len > 0) {
        printf("%c", *str);
        str++;
        len--;
    }
}
static void print_label_and_vstr(const char * label, const char * str, uint8_t len) {
    printf("%s: ", label);
    print_vstr(str, len);
    printf("\n");
}

void crypto_api_dumphex(const void * data, uint32_t data_size, uint32_t max_line_length) {
    dumphex(data, data_size, max_line_length);
}

void crypto_api_dump_pkcs_module_info(const PKCS_MODULE_INFO_t * module_info) {
    printf("  module path: %s\n", module_info->module);
}

void crypto_api_dump_pkcs_session_info(const PKCS_SESSION_INFO_t * session_info) {
    uint32_t n;

    printf("  slot id: %u\n", session_info->slot_id);
    printf("  flags: 0x%x\n", session_info->flags);
    printf("  users count: %u\n", session_info->users_count);
    for (n = 0; n < session_info->users_count; n++) {
        if (session_info->users[n].user_type_valid) {
            printf("  user %u type: %u\n", n, session_info->users[n].user_type);
        }
        if (session_info->users[n].user_pin_valid) {
            printf("  user %u pin: %s\n", n, session_info->users[n].user_pin);
        }
    }
}

void crypto_api_dump_pkcs_object_info(const PKCS_OBJECT_INFO_t * object_info) {
    uint32_t n;
    const char * attribute_name;

    printf("  attributes count: %u\n", object_info->attribute_count);
    for (n = 0; n < object_info->attribute_count; n++) {
        attribute_name = pkcs11_attribute_to_string(object_info->attributes[n].attribute_type);
        if (NULL == attribute_name) {
            attribute_name = "???";
        }
        printf("    %u: type = 0x%x (%s), value = ", n, object_info->attributes[n].attribute_type, attribute_name);
        switch (object_info->attributes[n].data_type) {
            case DATA_TYPE_BOOLEAN:
                printf("(BOOLEAN) %s\n", object_info->attributes[n].data.boolean ? "TRUE" : "FALSE");
                break;
            case DATA_TYPE_NUMBER:
                printf("(NUMBER) %lu (0x%lx)\n", object_info->attributes[n].data.number, object_info->attributes[n].data.number);
                break;
            case DATA_TYPE_STRING:
                printf("(STRING) %s\n", object_info->attributes[n].data.string.p);
                break;
            case DATA_TYPE_BLOB:
                printf("(BLOB)");
                dumphex(object_info->attributes[n].data.blob.p, object_info->attributes[n].data.blob.len, 0);
                printf("\n");
                break;
            default:
                printf("BAD DATA TYPE!\n");
                return;
        }
    }
}

void crypto_api_view_date_time(const DATE_AND_TIME_STAMP_t * date_time) {
    printf(" %04u-%02u-%02u %02u:%02u:%02u.%03u",
        date_time->year,
        date_time->month,
        date_time->day,
        date_time->hour,
        date_time->minutes,
        date_time->seconds,
        date_time->seconds_frac
    );
}
int crypto_api_view_certificate_request(const ESPERANTO_CERTIFICATE_REQUEST_t * pcertificate, bool silent, bool verbose) {
    const char * hash_alg_name;
    const char * ec_curve_name;

    if (!silent) {
        printf("Certificate request:\n");
        printf("  Version tag: 0x%08x\n", pcertificate->request_info.version_tag);

        printf("  Subject info:\n");
        printf("    Country: %c", (32 <= pcertificate->request_info.subject.country[0] && pcertificate->request_info.subject.country[0] < 127) ? pcertificate->request_info.subject.country[0] : '?');
        if (32 <= pcertificate->request_info.subject.country[1] && pcertificate->request_info.subject.country[1] < 127) {
            printf("%c\n", pcertificate->request_info.subject.country[1]);
        } else {
        	printf("\n");
        }
        print_label_and_vstr("    State: ", pcertificate->request_info.subject.state, pcertificate->request_info.subject.state_length);
        print_label_and_vstr("    City: ", pcertificate->request_info.subject.city, pcertificate->request_info.subject.city_length);
        print_label_and_vstr("    Organization: ", pcertificate->request_info.subject.organization, pcertificate->request_info.subject.organization_length);
        print_label_and_vstr("    Org unit: ", pcertificate->request_info.subject.org_unit, pcertificate->request_info.subject.org_unit_length);
        print_label_and_vstr("    Common name: ", pcertificate->request_info.subject.common_name, pcertificate->request_info.subject.common_name_length);
        print_label_and_vstr("    Serial number: ", pcertificate->request_info.subject.serial_number, pcertificate->request_info.subject.serial_number_length);
    }

    if (pcertificate->request_info.hash_algorithm != pcertificate->request_info_signature.hashAlg) {
        fprintf(stderr, "Error: hash algorithm mismatch!");
        return -1;
    }
    hash_alg_name = hash_algorithm_to_name(pcertificate->request_info.hash_algorithm, NULL);
    if (NULL == hash_alg_name) {
        fprintf(stderr, "Error: invalid or not supported hash algorithm!");
        return -1;
    }
    if (!silent) {
        printf("  Hash algorithm: %s\n", hash_alg_name);
    }
    
    if (pcertificate->request_info.subject_public_key.keyType != pcertificate->request_info_signature.keyType) {
        fprintf(stderr, "Error: public key type mismatch!");
        return -1;
    }
    
    switch (pcertificate->request_info.subject_public_key.keyType) {
    case PUBLIC_KEY_TYPE_RSA:
        if (!silent) {
            printf("  Public key type: RSA\n");
        }
        if (pcertificate->request_info.subject_public_key.rsa.keySize != pcertificate->request_info_signature.rsa.keySize) {
            fprintf(stderr, "Error: RSA key size mismatch!");
            return -1;
        }
        switch (pcertificate->request_info.subject_public_key.rsa.keySize) {
        case 2048:
        case 3072:
        case 4096:
            break;
        default:
            fprintf(stderr, "Error: invalid RSA key size!");
            return -1;
        }
        if (!silent) {
            printf("    Key size: %u\n", pcertificate->request_info.subject_public_key.rsa.keySize);
        }
        if (pcertificate->request_info.subject_public_key.rsa.pubExpSize > RSA_KEY_MAX_PUB_EXP_DATA_SIZE || pcertificate->request_info.subject_public_key.rsa.pubModSize > RSA_KEY_MAX_MODULUS_DATA_SIZE) {
            fprintf(stderr, "Error: corrupted RSA key data!");
            return -1;
        }
        if (verbose) {
            printf("    Public exponent:\n");
            dumphex(pcertificate->request_info.subject_public_key.rsa.pubExp, pcertificate->request_info.subject_public_key.rsa.pubExpSize, 32);
            printf("    Public modulus:\n");
            dumphex(pcertificate->request_info.subject_public_key.rsa.pubMod, pcertificate->request_info.subject_public_key.rsa.pubModSize, 32);
        }
        break;

    case PUBLIC_KEY_TYPE_EC:
        if (!silent) {
            printf("  Public key type: EC\n");
        }
        if (pcertificate->request_info.subject_public_key.ec.curveID != pcertificate->request_info_signature.ec.curveID) {
            fprintf(stderr, "Error: EC key curve ID mismatch!");
            return -1;
        }
        ec_curve_name = ec_curve_id_to_name(pcertificate->request_info.subject_public_key.ec.curveID);
        if (NULL == ec_curve_name) {
            fprintf(stderr, "Error: invalid EC key curve id!");
            return -1;
        }
        if (!silent) {
            printf("    Curve: %s\n", ec_curve_name);
        }
        if (pcertificate->request_info.subject_public_key.ec.pXsize > ECC_KEY_MAX_POINT_DATA_SIZE || pcertificate->request_info.subject_public_key.ec.pYsize > ECC_KEY_MAX_POINT_DATA_SIZE) {
            fprintf(stderr, "Error: corrupted EC key public key point!");
            return -1;
        }
        if (verbose) {
            printf("    Q.X:\n");
            dumphex(pcertificate->request_info.subject_public_key.ec.pX, pcertificate->request_info.subject_public_key.ec.pXsize, 32);
            printf("    Q.Y:\n");
            dumphex(pcertificate->request_info.subject_public_key.ec.pY, pcertificate->request_info.subject_public_key.ec.pYsize, 32);
        }
        break;

    default:
        fprintf(stderr, "Error: invalid public key type!");
        return -1;
    }

    if (!silent) {
        printf("  Subject key identifier:\n");
        dumphex(pcertificate->request_info.subject_key_identifier.bytes, sizeof(pcertificate->request_info.subject_key_identifier.bytes), 32);
    }

    switch (pcertificate->request_info.subject_public_key.keyType) {
    case PUBLIC_KEY_TYPE_RSA:
        if (pcertificate->request_info_signature.rsa.sigSize > RSA_KEY_MAX_MODULUS_DATA_SIZE) {
            fprintf(stderr, "Error: corrupted RSA signature data!");
            return -1;
        }
        if (verbose) {
            printf("    Signature:\n");
            dumphex(pcertificate->request_info_signature.rsa.signature, pcertificate->request_info_signature.rsa.sigSize, 32);
        }
        break;

    case PUBLIC_KEY_TYPE_EC:
        if (pcertificate->request_info_signature.ec.rSize > ECC_KEY_MAX_POINT_DATA_SIZE || pcertificate->request_info_signature.ec.sSize > ECC_KEY_MAX_POINT_DATA_SIZE) {
            fprintf(stderr, "Error: corrupted EC signature data!");
            return -1;
        }
        if (verbose) {
            printf("    R:\n");
            dumphex(pcertificate->request_info_signature.ec.r, pcertificate->request_info_signature.ec.rSize, 32);
            printf("    S:\n");
            dumphex(pcertificate->request_info_signature.ec.s, pcertificate->request_info_signature.ec.sSize, 32);
        }
        break;

    default:
        return -1;
    }

    return 0;
}

int crypto_api_view_signature(const PUBLIC_SIGNATURE_t * signature, bool silent, bool verbose) {
    switch (signature->keyType) {
    case PUBLIC_KEY_TYPE_RSA:
        if (signature->rsa.sigSize > RSA_KEY_MAX_MODULUS_DATA_SIZE) {
            fprintf(stderr, "Error: corrupted RSA signature data!");
            return -1;
        }
        if (!silent && verbose) {
            printf("    Signature:\n");
            dumphex(signature->rsa.signature, signature->rsa.sigSize, 32);
        }
        break;

    case PUBLIC_KEY_TYPE_EC:
        if (signature->ec.rSize > ECC_KEY_MAX_POINT_DATA_SIZE || signature->ec.sSize > ECC_KEY_MAX_POINT_DATA_SIZE) {
            fprintf(stderr, "Error: corrupted EC signature data!");
            return -1;
        }
        if (verbose) {
            printf("    R:\n");
            dumphex(signature->ec.r, signature->ec.rSize, 32);
            printf("    S:\n");
            dumphex(signature->ec.s, signature->ec.sSize, 32);
        }
        break;

    default:
        return -1;
    }

    return 0;
}

int crypto_api_view_certificate(const ESPERANTO_CERTIFICATE_t * pcertificate, bool silent, bool verbose) {
    const char * hash_alg_name;
    const char * ec_curve_name;

    if (!silent) {
        printf("Certificate:\n");
        printf("  Version tag: 0x%08x\n", pcertificate->certificate_info.version_tag);
        printf("  Serial number: %u (0x%08x)\n", pcertificate->certificate_info.serial_number, pcertificate->certificate_info.serial_number);

        printf("  Issuer info:\n");
        printf("    Country: %c", (32 <= pcertificate->certificate_info.issuer.country[0] && pcertificate->certificate_info.issuer.country[0] < 127) ? pcertificate->certificate_info.issuer.country[0] : '?');
        if (32 <= pcertificate->certificate_info.issuer.country[1] && pcertificate->certificate_info.issuer.country[1] < 127) {
            printf("%c\n", pcertificate->certificate_info.issuer.country[1]);
        } else {
        	printf("\n");
        }
        print_label_and_vstr("    State: ", pcertificate->certificate_info.issuer.state, pcertificate->certificate_info.issuer.state_length);
        print_label_and_vstr("    City: ", pcertificate->certificate_info.issuer.city, pcertificate->certificate_info.issuer.city_length);
        print_label_and_vstr("    Organization: ", pcertificate->certificate_info.issuer.organization, pcertificate->certificate_info.issuer.organization_length);
        print_label_and_vstr("    Org unit: ", pcertificate->certificate_info.issuer.org_unit, pcertificate->certificate_info.issuer.org_unit_length);
        print_label_and_vstr("    Common name: ", pcertificate->certificate_info.issuer.common_name, pcertificate->certificate_info.issuer.common_name_length);
        print_label_and_vstr("    Serial number: ", pcertificate->certificate_info.issuer.serial_number, pcertificate->certificate_info.issuer.serial_number_length);
    }

    if (pcertificate->certificate_info.hash_algorithm != pcertificate->certificate_info_signature.hashAlg) {
        fprintf(stderr, "Error: hash algorithm mismatch!");
        return -1;
    }
    hash_alg_name = hash_algorithm_to_name(pcertificate->certificate_info.hash_algorithm, NULL);
    if (NULL == hash_alg_name) {
        fprintf(stderr, "Error: invalid or not supported hash algorithm!");
        return -1;
    }
    if (!silent) {
        printf("  Hash algorithm: %s\n", hash_alg_name);
    }
    
    if (pcertificate->certificate_info.signing_key_type != pcertificate->certificate_info_signature.keyType) {
        fprintf(stderr, "Error: issuer signing key type mismatch!");
        return -1;
    }
    switch (pcertificate->certificate_info.signing_key_type) {
    case PUBLIC_KEY_TYPE_RSA:
        if (pcertificate->certificate_info.keySize != pcertificate->certificate_info_signature.rsa.keySize) {
            fprintf(stderr, "Error: issuer signing RSA key size mismatch!");
            return -1;
        }
        break;
    case PUBLIC_KEY_TYPE_EC:
        if (pcertificate->certificate_info.curveID != pcertificate->certificate_info_signature.ec.curveID) {
            fprintf(stderr, "Error: issuer signing EC key curve ID mismatch!");
            return -1;
        }
        break;
    default:
        fprintf(stderr, "Error: invalid issuer signing key type!");
        return -1;
    }

    if (verbose) {
        printf("  Valid from: ");
        crypto_api_view_date_time(&pcertificate->certificate_info.valid_from);
        printf("\n");
        printf("  Valid until: ");
        crypto_api_view_date_time(&pcertificate->certificate_info.valid_until);
        printf("\n");
    }

    if (!silent) {
        printf("  Subject info:\n");
        printf("    Country: %c", (32 <= pcertificate->certificate_info.subject.country[0] && pcertificate->certificate_info.subject.country[0] < 127) ? pcertificate->certificate_info.subject.country[0] : '?');
        if (32 <= pcertificate->certificate_info.subject.country[1] && pcertificate->certificate_info.subject.country[1] < 127) {
            printf("%c\n", pcertificate->certificate_info.subject.country[1]);
        } else {
        	printf("\n");
        }
        print_label_and_vstr("    State: ", pcertificate->certificate_info.subject.state, pcertificate->certificate_info.subject.state_length);
        print_label_and_vstr("    City: ", pcertificate->certificate_info.subject.city, pcertificate->certificate_info.subject.city_length);
        print_label_and_vstr("    Organization: ", pcertificate->certificate_info.subject.organization, pcertificate->certificate_info.subject.organization_length);
        print_label_and_vstr("    Org unit: ", pcertificate->certificate_info.subject.org_unit, pcertificate->certificate_info.subject.org_unit_length);
        print_label_and_vstr("    Common name: ", pcertificate->certificate_info.subject.common_name, pcertificate->certificate_info.subject.common_name_length);
        print_label_and_vstr("    Serial number: ", pcertificate->certificate_info.subject.serial_number, pcertificate->certificate_info.subject.serial_number_length);
    }

    switch (pcertificate->certificate_info.subject_public_key.keyType) {
    case PUBLIC_KEY_TYPE_RSA:
        if (!silent) {
            printf("  Subject public key type: RSA\n");
        }
        switch (pcertificate->certificate_info.subject_public_key.rsa.keySize) {
        case 2048:
        case 3072:
        case 4096:
            break;
        default:
            fprintf(stderr, "Error: invalid RSA key size!");
            return -1;
        }
        if (!silent) {
            printf("    Key size: %u\n", pcertificate->certificate_info.subject_public_key.rsa.keySize);
        }
        if (pcertificate->certificate_info.subject_public_key.rsa.pubExpSize > RSA_KEY_MAX_PUB_EXP_DATA_SIZE || pcertificate->certificate_info.subject_public_key.rsa.pubModSize > RSA_KEY_MAX_MODULUS_DATA_SIZE) {
            fprintf(stderr, "Error: corrupted RSA key data!");
            return -1;
        }
        if (verbose) {
            printf("    Public exponent:\n");
            dumphex(pcertificate->certificate_info.subject_public_key.rsa.pubExp, pcertificate->certificate_info.subject_public_key.rsa.pubExpSize, 32);
            printf("    Public modulus:\n");
            dumphex(pcertificate->certificate_info.subject_public_key.rsa.pubMod, pcertificate->certificate_info.subject_public_key.rsa.pubModSize, 32);
        }
        break;

    case PUBLIC_KEY_TYPE_EC:
        if (!silent) {
            printf("  Subject public key type: EC\n");
        }
        ec_curve_name = ec_curve_id_to_name(pcertificate->certificate_info.subject_public_key.ec.curveID);
        if (NULL == ec_curve_name) {
            fprintf(stderr, "Error: invalid EC key curve id!");
            return -1;
        }
        if (!silent) {
            printf("    Curve: %s\n", ec_curve_name);
        }
        if (pcertificate->certificate_info.subject_public_key.ec.pXsize > ECC_KEY_MAX_POINT_DATA_SIZE || pcertificate->certificate_info.subject_public_key.ec.pYsize > ECC_KEY_MAX_POINT_DATA_SIZE) {
            fprintf(stderr, "Error: corrupted EC key public key point!");
            return -1;
        }
        if (verbose) {
            printf("    Q.X:\n");
            dumphex(pcertificate->certificate_info.subject_public_key.ec.pX, pcertificate->certificate_info.subject_public_key.ec.pXsize, 32);
            printf("    Q.Y:\n");
            dumphex(pcertificate->certificate_info.subject_public_key.ec.pY, pcertificate->certificate_info.subject_public_key.ec.pYsize, 32);
        }
        break;

    default:
        fprintf(stderr, "Error: invalid public key type!");
        return -1;
    }

    if (!silent) {
        printf("  Is CA: %u (0x%08x)\n", pcertificate->certificate_info.is_CA, pcertificate->certificate_info.is_CA);
        printf("  CA depth: %u (0x%08x)\n", pcertificate->certificate_info.ca_depth, pcertificate->certificate_info.ca_depth);
        printf("  X509 attributes: %u (0x%08x)\n", pcertificate->certificate_info.x509_attributes, pcertificate->certificate_info.x509_attributes);
        printf("  SCID: %u (0x%02x)\n", pcertificate->certificate_info.special.reserved, pcertificate->certificate_info.special.reserved);
        printf("  Esperanto attributes: %u (0x%08x)\n", pcertificate->certificate_info.esperanto_attributes, pcertificate->certificate_info.esperanto_attributes);
        printf("  Esperanto designation: %u (0x%08x)\n", pcertificate->certificate_info.esperanto_designation, pcertificate->certificate_info.esperanto_designation);
        printf("  Revocation counter: %u (0x%08x)\n", pcertificate->certificate_info.revocation_counter, pcertificate->certificate_info.revocation_counter);
    }

    if (!silent) {
        printf("  Issuer key identifier:\n");
        dumphex(pcertificate->certificate_info.issuer_key_identifier.bytes, sizeof(pcertificate->certificate_info.issuer_key_identifier.bytes), 32);
    }

    if (!silent) {
        printf("  Subject key identifier:\n");
        dumphex(pcertificate->certificate_info.subject_key_identifier.bytes, sizeof(pcertificate->certificate_info.subject_key_identifier.bytes), 32);
    }

    return crypto_api_view_signature(&pcertificate->certificate_info_signature, silent, verbose);
}

int crypto_view_signed_elf_header(const ESPERANTO_IMAGE_FILE_HEADER_t * esperanto_image_header, bool silent, bool verbose, bool decrypted, bool display_image_header, bool display_file_header, bool do_not_check_signature) {
    uint32_t n;
    const char * mac_alg_name;
    uint32_t mac_size;
    const char * image_type_string;
    uint32_t code_and_data_hash_size;
    bool signature_ok;
    union {
        uint32_t u32;
        struct {
            uint8_t revision;
            uint8_t minor;
            uint8_t major;
            uint8_t reserved;
        };
    } fv;
    char git_version[MAX_GIT_VERSION_LENGTH+1];

    if (display_file_header) {
        if (ESPERANTO_MAC_TYPE_INVALID == esperanto_image_header->info.mac_type) {
            if (0 != (ESPERANTO_IMAGE_FILE_HEADER_FLAGS_ENCRYPTED & esperanto_image_header->info.file_header_flags)) {
                fprintf(stderr, "Error in crypto_view_signed_elf_header: invalid mac algorithm id 0x%x in an encrypted image!\n", esperanto_image_header->info.mac_type);
                return -1;
            }
        } else {
            if (0 == (ESPERANTO_IMAGE_FILE_HEADER_FLAGS_ENCRYPTED & esperanto_image_header->info.file_header_flags)) {
                fprintf(stderr, "Error in crypto_view_signed_elf_header: invalid mac algorithm id 0x%x in a non-encrypted image!\n", esperanto_image_header->info.mac_type);
                return -1;
            }
            mac_alg_name = mac_algorithm_to_name(esperanto_image_header->info.mac_type, &mac_size);
            if (NULL == mac_alg_name) {
                fprintf(stderr, "Error in crypto_view_signed_elf_header: invalid mac algorithm id 0x%x!\n", esperanto_image_header->info.mac_type);
                return -1;
            }
        }
        if (!silent) {
            printf("======== FILE HEADER START ========\n");
            printf("file_header_flags: 0x%x\n", esperanto_image_header->info.file_header_flags);
            if (ESPERANTO_MAC_TYPE_INVALID == esperanto_image_header->info.mac_type) {
                printf("mac_type: N/A\n");
                printf("encryption_IV: N/A\n");
                if (verbose) {
                    printf("encrypted_code_and_data_hash: N/A\n");
                    printf("MAC signature: N/A\n");
                }
            } else {
                printf("mac_type: %s\n", mac_alg_name);
                printf("encryption_IV:\n");
                crypto_api_dumphex(esperanto_image_header->info.encryption_IV, sizeof(esperanto_image_header->info.encryption_IV), 32);
                if (verbose) {
                    printf("encrypted_code_and_data_hash:\n");
                    crypto_api_dumphex(esperanto_image_header->info.encrypted_code_and_data_hash, sizeof(esperanto_image_header->info.encrypted_code_and_data_hash), 32);
                    printf("MAC signature:\n");
                    crypto_api_dumphex(esperanto_image_header->MAC, mac_size, 32);
                }
            }
        }
	if (do_not_check_signature) {
	    if (!silent) {
		printf("Certificate Invalid (Signature Disabled)\n");
	    }
	} else {
	    if (0 != crypto_api_view_certificate(&esperanto_image_header->info.signing_certificate, silent, verbose)) {
		fprintf(stderr, "Error in crypto_view_signed_elf_header: crypto_api_view_certificate() failed!\n");
		return -1;
	    }
	}

        if (!silent) {
            printf("========= FILE HEADER END =========\n");
        }
    }

    if (display_image_header) {
        image_type_string = executable_image_type_to_type_name(esperanto_image_header->info.image_info_and_signaure.info.public_info.image_type);
        if (NULL == image_type_string) {
            fprintf(stderr, "Error in crypto_view_signed_elf_header: Invalid or not supported image type 0x%x!\n", esperanto_image_header->info.image_info_and_signaure.info.public_info.image_type);
            return -1;
        }
        switch (esperanto_image_header->info.image_info_and_signaure.info.public_info.code_and_data_hash_algorithm) {
        case HASH_ALG_SHA2_256:
        case HASH_ALG_SHA3_256:
            code_and_data_hash_size = 256 / 8;
            break;
        case HASH_ALG_SHA2_384:
        case HASH_ALG_SHA3_384:
            code_and_data_hash_size = 384 / 8;
            break;
        case HASH_ALG_SHA2_512:
        case HASH_ALG_SHA3_512:
            code_and_data_hash_size = 512 / 8;
            break;
        default:
            fprintf(stderr, "Error in crypto_view_signed_elf_header: Invalid or not supported hash algorithm 0x%x!\n", esperanto_image_header->info.image_info_and_signaure.info.public_info.code_and_data_hash_algorithm);
            return -1;
        }

        if (do_not_check_signature) {
            if (!silent) {
                printf("Image signature ignored.\n");
            }
        } else {
            if (decrypted || 0 == (ESPERANTO_IMAGE_FILE_HEADER_FLAGS_ENCRYPTED & esperanto_image_header->info.file_header_flags)) {
                if (0 != crypto_api_verify_with_public_key(&esperanto_image_header->info.image_info_and_signaure.info_signature, 
                                                        &esperanto_image_header->info.image_info_and_signaure.info, 
                                                        sizeof(esperanto_image_header->info.image_info_and_signaure.info), 
                                                        &esperanto_image_header->info.signing_certificate.certificate_info.subject_public_key,
                                                        &signature_ok)) {
                    fprintf(stderr, "crypto_view_signed_elf_header: crypto_api_verify_with_public_key() failed!\n");
                    return -1;
                }
                if (!signature_ok) {
                    if (!silent) {
                        printf("Image signature check failed!\n");
                    }
                    return -1;
                } else {
                    if (!silent) {
                        printf("Image signature OK!\n");
                    }
                }
            }
        }
        
        if (!silent) {
            printf("======== SIGNED IMAGE HEADER START ========\n");
            printf("image_type: 0x%x (%s)\n", esperanto_image_header->info.image_info_and_signaure.info.public_info.image_type, image_type_string);
            fv.u32 = esperanto_image_header->info.image_info_and_signaure.info.public_info.file_version;
            printf("file_version: %u.%u.%u (0x%08x)\n", fv.major, fv.minor, fv.revision, fv.u32);
            printf("date_time_stamp: ");
            crypto_api_view_date_time(&esperanto_image_header->info.image_info_and_signaure.info.public_info.fileDateAndTimeStamp);
            printf("\n");
            printf("image_info_flags: 0x%x\n", esperanto_image_header->info.image_info_and_signaure.info.public_info.image_info_flags);
            printf("revocation_counter: %u\n", esperanto_image_header->info.image_info_and_signaure.info.public_info.revocation_counter);
            printf("hash_algorithm: 0x%x (%s)\n", esperanto_image_header->info.image_info_and_signaure.info.public_info.code_and_data_hash_algorithm, hash_algorithm_to_name(esperanto_image_header->info.image_info_and_signaure.info.public_info.code_and_data_hash_algorithm, NULL));
            if (verbose) {
                printf("code_and_data_hash:\n");
                crypto_api_dumphex(esperanto_image_header->info.image_info_and_signaure.info.public_info.code_and_data_hash, code_and_data_hash_size, 32);
            }
            printf("GIT_hash: ");
            crypto_api_dump_githash(false, esperanto_image_header->info.image_info_and_signaure.info.public_info.git_hash);
            memcpy(git_version, esperanto_image_header->info.image_info_and_signaure.info.public_info.git_version, MAX_GIT_VERSION_LENGTH);
            git_version[MAX_GIT_VERSION_LENGTH] = 0;
            printf("\nGIT_version: %s\n", git_version);

            if (decrypted || 0 == (ESPERANTO_IMAGE_FILE_HEADER_FLAGS_ENCRYPTED & esperanto_image_header->info.file_header_flags)) {
                printf("Exec address: 0x%08x_%08x\n", esperanto_image_header->info.image_info_and_signaure.info.secret_info.exec_address_hi, esperanto_image_header->info.image_info_and_signaure.info.secret_info.exec_address_lo);
                printf("Exec flags: 0x%08x\n", esperanto_image_header->info.image_info_and_signaure.info.secret_info.exec_flags);
                printf("Total code+data size: %u (0x%x)\n", esperanto_image_header->info.image_info_and_signaure.info.public_info.code_and_data_size, esperanto_image_header->info.image_info_and_signaure.info.public_info.code_and_data_size);
                printf("Load regions count: %u\n", esperanto_image_header->info.image_info_and_signaure.info.secret_info.load_regions_count);
                for (n = 0; n < esperanto_image_header->info.image_info_and_signaure.info.secret_info.load_regions_count; n++) {
                    printf("  region %u:", n);
                    printf("    offset=0x%08x", esperanto_image_header->info.image_info_and_signaure.info.secret_info.load_regions[n].region_offset);
                    printf("    address=0x%08x_%08x", esperanto_image_header->info.image_info_and_signaure.info.secret_info.load_regions[n].load_address_hi, esperanto_image_header->info.image_info_and_signaure.info.secret_info.load_regions[n].load_address_lo);
                    printf("    load_size=0x%08x", esperanto_image_header->info.image_info_and_signaure.info.secret_info.load_regions[n].load_size);
                    printf("    memory_size=0x%08x", esperanto_image_header->info.image_info_and_signaure.info.secret_info.load_regions[n].memory_size);
                    printf("    flags=0x%08x", esperanto_image_header->info.image_info_and_signaure.info.secret_info.load_regions[n].region_flags);
                    printf("\n");
                }
            } else {
                printf("*** Skipping encrypted content ***\n");
            }
            printf("========= SIGNED IMAGE HEADER END =========\n");
            if (verbose && !do_not_check_signature) {
                printf("public key signature:\n");
                if (0 != crypto_api_view_signature(&esperanto_image_header->info.image_info_and_signaure.info_signature, silent, verbose)) {
                    fprintf(stderr, "Error:  crypto_api_view_signature() failed!\n");
                    return -1;
                }
            }
        }
    }

    return 0;
}

int crypto_view_signed_raw_header(const ESPERANTO_RAW_IMAGE_FILE_HEADER_t * esperanto_image_header, bool silent, bool verbose, bool display_image_header, bool display_file_header, bool do_not_check_signature) {
    const char * mac_alg_name;
    uint32_t mac_size;
    const char * image_type_string;
    uint32_t raw_data_hash_size;
    bool signature_ok;
    union {
        uint32_t u32;
        struct {
            uint8_t revision;
            uint8_t minor;
            uint8_t major;
            uint8_t reserved;
        };
    } fv;
    char git_version[MAX_GIT_VERSION_LENGTH+1];

    if (display_file_header) {
        if (ESPERANTO_MAC_TYPE_INVALID == esperanto_image_header->info.mac_type) {
            if (0 != (ESPERANTO_RAW_IMAGE_FILE_HEADER_FLAGS_ENCRYPTED & esperanto_image_header->info.file_header_flags)) {
                fprintf(stderr, "Error in crypto_view_signed_raw_header: invalid mac algorithm id 0x%x in an encrypted image!\n", esperanto_image_header->info.mac_type);
                return -1;
            }
        } else {
            if (0 == (ESPERANTO_RAW_IMAGE_FILE_HEADER_FLAGS_ENCRYPTED & esperanto_image_header->info.file_header_flags)) {
                fprintf(stderr, "Error in crypto_view_signed_raw_header: invalid mac algorithm id 0x%x in a non-encrypted image!\n", esperanto_image_header->info.mac_type);
                return -1;
            }
            mac_alg_name = mac_algorithm_to_name(esperanto_image_header->info.mac_type, &mac_size);
            if (NULL == mac_alg_name) {
                fprintf(stderr, "Error in crypto_view_signed_raw_header: invalid mac algorithm id 0x%x!\n", esperanto_image_header->info.mac_type);
                return -1;
            }
        }
        if (!silent) {
            printf("======== FILE HEADER START ========\n");
            printf("file_header_flags: 0x%x\n", esperanto_image_header->info.file_header_flags);
            if (ESPERANTO_MAC_TYPE_INVALID == esperanto_image_header->info.mac_type) {
                printf("mac_type: N/A\n");
                printf("encryption_IV: N/A\n");
                if (verbose) {
                    printf("encrypted_code_and_data_hash: N/A\n");
                    printf("MAC signature: N/A\n");
                }
            } else {
                printf("mac_type: %s\n", mac_alg_name);
                printf("encryption_IV:\n");
                crypto_api_dumphex(esperanto_image_header->info.encryption_IV, sizeof(esperanto_image_header->info.encryption_IV), 32);
                if (verbose) {
                    printf("encrypted_code_and_data_hash:\n");
                    crypto_api_dumphex(esperanto_image_header->info.encrypted_code_and_data_hash, sizeof(esperanto_image_header->info.encrypted_code_and_data_hash), 32);
                    printf("MAC signature:\n");
                    crypto_api_dumphex(esperanto_image_header->MAC, mac_size, 32);
                }
            }
        }
	if (do_not_check_signature) {
	    if (!silent) {
		printf("Certificate Invalid (Signature Disabled)\n");
	    }
	} else {
	    if (0 != crypto_api_view_certificate(&esperanto_image_header->info.signing_certificate, silent, verbose)) {
		fprintf(stderr, "Error in crypto_view_signed_raw_header: crypto_api_view_certificate() failed!\n");
		return -1;
	    }
	}
        if (!silent) {
            printf("========= FILE HEADER END =========\n");
        }
    }

    if (display_image_header) {
        image_type_string = raw_image_type_to_type_name(esperanto_image_header->info.image_info_and_signaure.info.image_type);
        if (NULL == image_type_string) {
            fprintf(stderr, "Error in crypto_view_signed_raw_header: Invalid or not supported image type 0x%x!\n", esperanto_image_header->info.image_info_and_signaure.info.image_type);
            return -1;
        }
        switch (esperanto_image_header->info.image_info_and_signaure.info.raw_image_hash_algorithm) {
        case HASH_ALG_SHA2_256:
        case HASH_ALG_SHA3_256:
            raw_data_hash_size = 256 / 8;
            break;
        case HASH_ALG_SHA2_384:
        case HASH_ALG_SHA3_384:
            raw_data_hash_size = 384 / 8;
            break;
        case HASH_ALG_SHA2_512:
        case HASH_ALG_SHA3_512:
            raw_data_hash_size = 512 / 8;
            break;
        default:
            fprintf(stderr, "Error in crypto_view_signed_raw_header: Invalid or not supported hash algorithm 0x%x!\n", esperanto_image_header->info.image_info_and_signaure.info.raw_image_hash_algorithm);
            return -1;
        }

        if (do_not_check_signature) {
            if (!silent) {
                printf("Image signature ignored.\n");
            }
        } else {
            if (0 != crypto_api_verify_with_public_key(&esperanto_image_header->info.image_info_and_signaure.info_signature, 
                                                        &esperanto_image_header->info.image_info_and_signaure.info, 
                                                        sizeof(esperanto_image_header->info.image_info_and_signaure.info), 
                                                        &esperanto_image_header->info.signing_certificate.certificate_info.subject_public_key,
                                                        &signature_ok)) {
                fprintf(stderr, "crypto_view_signed_raw_header: crypto_api_verify_with_public_key() failed!\n");
                return -1;
            }
            if (!signature_ok) {
                if (!silent) {
                    printf("Image signature check failed!\n");
                }
                return -1;
            } else {
                if (!silent) {
                    printf("Image signature OK!\n");
                }
            }
        }
        
        if (!silent) {
            printf("======== SIGNED IMAGE HEADER START ========\n");
            printf("image_type: 0x%x (%s)\n", esperanto_image_header->info.image_info_and_signaure.info.image_type, image_type_string);
            fv.u32 = esperanto_image_header->info.image_info_and_signaure.info.file_version;
            printf("file_version: %u.%u.%u (0x%08x)\n", fv.major, fv.minor, fv.revision, fv.u32);
            printf("date_time_stamp: ");
            crypto_api_view_date_time(&esperanto_image_header->info.image_info_and_signaure.info.fileDateAndTimeStamp);
            printf("\n");
            printf("image_info_flags: 0x%x\n", esperanto_image_header->info.image_info_and_signaure.info.image_info_flags);
            printf("revocation_counter: %u\n", esperanto_image_header->info.image_info_and_signaure.info.revocation_counter);
            printf("hash_algorithm: 0x%x (%s)\n", esperanto_image_header->info.image_info_and_signaure.info.raw_image_hash_algorithm, hash_algorithm_to_name(esperanto_image_header->info.image_info_and_signaure.info.raw_image_hash_algorithm, NULL));
            if (verbose) {
                printf("code_and_data_hash:\n");
                crypto_api_dumphex(esperanto_image_header->info.image_info_and_signaure.info.raw_image_hash, raw_data_hash_size, 32);
            }
            printf("GIT_hash: ");
            crypto_api_dump_githash(false, esperanto_image_header->info.image_info_and_signaure.info.git_hash);
            memcpy(git_version, esperanto_image_header->info.image_info_and_signaure.info.git_version, MAX_GIT_VERSION_LENGTH);
            git_version[MAX_GIT_VERSION_LENGTH] = 0;
            printf("\nGIT_version: %s\n", git_version);

            printf("========= SIGNED IMAGE HEADER END =========\n");
            if (verbose && !do_not_check_signature) {
                printf("public key signature:\n");
                if (0 != crypto_api_view_signature(&esperanto_image_header->info.image_info_and_signaure.info_signature, silent, verbose)) {
                    fprintf(stderr, "Error:  crypto_api_view_signature() failed!\n");
                    return -1;
                }
            }
        }
    }

    return 0;
}
