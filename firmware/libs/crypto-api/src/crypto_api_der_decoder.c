#include "crypto_api_der_decoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum IDENTIFIER_CLASS {
    IDENTIFIER_CLASS_UNIVERSAL,
    IDENTIFIER_CLASS_APPLICATION,
    IDENTIFIER_CLASS_CONTEXT_SPECIFIC,
    IDENTIFIER_CLASS_PRIVATE
} IDENTIFIER_CLASS_t;

#define IDENTIFIER_OCTET_STRING     4
#define IDENTIFIER_OCTET_INTEGER    2
#define IDENTIFIER_OCTET_SEQUENCE   0x30

static int parse_der(const uint8_t * der_data, uint32_t der_data_size, uint32_t * ptag_number, IDENTIFIER_CLASS_t* pclass, uint32_t * pdata_length, uint32_t * pdata_offset) {
    uint32_t tag_number;
    IDENTIFIER_CLASS_t class;
    uint32_t length_octets;
    uint32_t data_length;
    uint32_t offset = 0;
    bool final_identifier_octet;

    if (NULL == der_data || 0 == der_data_size) {
            fprintf(stderr, "ERROR in parse_der: invalid arguments!\n");
        return -1;
    }

    ///////////////////////////////////////////////////////////////////////////
    // decode TAG CLASS
    ///////////////////////////////////////////////////////////////////////////

    if (0x80 & der_data[offset]) {
        if (0x40 & der_data[offset]) {
            class = IDENTIFIER_CLASS_PRIVATE;
        } else {
            class = IDENTIFIER_CLASS_CONTEXT_SPECIFIC;
        }
    } else {
        if (0x40 & der_data[offset]) {
            class = IDENTIFIER_CLASS_APPLICATION;
        } else {
            class = IDENTIFIER_CLASS_UNIVERSAL;
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // decode TAG NUMBER
    ///////////////////////////////////////////////////////////////////////////

    if (0x20 & der_data[offset]) {
        // constructed tag encoding
        if (0x1F != der_data[offset]) {
            fprintf(stderr, "ERROR in parse_der: first byte of constructed tag number is not b11111!\n");
            return -1;
        }
        tag_number = 0;
        offset++;
        
        do {
            if (offset >= der_data_size) {
                fprintf(stderr, "ERROR in parse_der: der_data_size too small!\n");
                return -1;
            }

            if (0x80 & der_data[offset]) {
                final_identifier_octet = false;
            } else {
                final_identifier_octet = true;
            }

            tag_number = tag_number + (der_data[offset] & 0x7F);
            offset++;
        } while (!final_identifier_octet);
    } else {
        // primitive tag encoding
        tag_number = der_data[0] & 0x1F;
        if (0x1F == tag_number) {
            fprintf(stderr, "ERROR in parse_der: primitve tag number is b11111!\n");
            return -1;
        }
        offset = 1;
    }

    ///////////////////////////////////////////////////////////////////////////
    // decode DATA LENGTH
    ///////////////////////////////////////////////////////////////////////////

    if (offset >= der_data_size) {
        fprintf(stderr, "ERROR in parse_der: der_data_size too small!\n");
        return -1;
    }
    if (0x80 & der_data[offset]) {
        // definite, long form
        length_octets = der_data[1] & 0x7F;
        if (0 == length_octets) {
            fprintf(stderr, "ERROR in parse_der: length_octets is ZERO!\n");
            return -1;
        }
        if (length_octets > 4) {
            fprintf(stderr, "ERROR in parse_der: length_octets value %u is not supported!\n", length_octets);
            return -1;
        }
        data_length = 0;
        offset++;
        do {
            if (offset >= der_data_size) {
                fprintf(stderr, "ERROR in parse_der: der_data_size too small!\n");
                return -1;
            }
            data_length = data_length << 8u;
            data_length = data_length | der_data[offset];
            offset++;
            length_octets--;
        } while (length_octets > 0);
    } else {
        // definite, short form
        data_length = der_data[offset] & 0x7F;
        offset++;
    }

    if (NULL != ptag_number) {
        *ptag_number = tag_number;
    }
    if (NULL != pclass) {
        *pclass = class;
    }
    if (NULL != pdata_length) {
        *pdata_length = data_length;
    }
    if (NULL != pdata_offset) {
        *pdata_offset = offset;
    }

    return 0;
}

int crypto_api_decode_ec_public_key(const uint8_t * der_data, uint32_t der_data_size, PUBLIC_KEY_EC_t * public_key_data) {
    //int rval;
    uint32_t tag_number;
    IDENTIFIER_CLASS_t class;
    uint32_t data_length;
    uint32_t data_offset;
    uint32_t expected_data_length, expected_point_length, point_length;
    bool two_coordinates;
    
    if (NULL == der_data || 0 == der_data_size || NULL == public_key_data) {
        fprintf(stderr, "ERROR in crypto_api_decode_ec_public_key: invalid arguments!\n");
        return -1;
    }

    memset(public_key_data->pX, 0, sizeof(public_key_data->pX));
    memset(public_key_data->pY, 0, sizeof(public_key_data->pY));

    switch (public_key_data->curveID) {
    case EC_KEY_CURVE_NIST_P256:
        expected_point_length = 32;
        two_coordinates = true;
        break;
    case EC_KEY_CURVE_NIST_P384:
        expected_point_length = 48;
        two_coordinates = true;
        break;
    case EC_KEY_CURVE_NIST_P521:
        expected_point_length = 68;
        two_coordinates = true;
        break;
    case EC_KEY_CURVE_CURVE25519:
        expected_point_length = 32;
        two_coordinates = false;
        break;
    case EC_KEY_CURVE_EDWARDS25519:
        expected_point_length = 32;
        two_coordinates = false;
        break;
    default:
        fprintf(stderr, "ERROR in crypto_api_decode_ec_public_key: Invalid/unknown EC CURVE ID!\n");
        return -1;
    }

    if (0 != parse_der(der_data, der_data_size, &tag_number, &class, &data_length, &data_offset)) {
        fprintf(stderr, "ERROR in crypto_api_decode_ec_public_key: parse_der() failed!\n");
        return -1;
    }

    if (der_data_size != (data_length + data_offset)) {
        fprintf(stderr, "ERROR in crypto_api_decode_ec_public_key: bad ECC point data!\n");
        return -1;
    }

    if (IDENTIFIER_CLASS_UNIVERSAL != class || IDENTIFIER_OCTET_STRING != tag_number) {
        fprintf(stderr, "ERROR in crypto_api_decode_ec_public_key: bad ECC point data octet string!\n");
        return -1;
    }

    if (two_coordinates) {
        expected_data_length = 1 + 2 * expected_point_length;
        if ((data_length < 3) || (data_length > expected_data_length) || (0 == (0x1 & data_length))) {
            fprintf(stderr, "ERROR in crypto_api_decode_ec_public_key: bad ECC point data octet string length!\n");
            return -1;
        }
        point_length = (data_length - 1) / 2;
        public_key_data->pXsize = point_length;
        memcpy(public_key_data->pX, der_data + data_offset + 1, point_length);
        public_key_data->pYsize = point_length;
        memcpy(public_key_data->pY, der_data + data_offset + 1 + point_length, point_length);
    } else {
        expected_data_length = expected_point_length;
        if ((data_length < 1) || (data_length > expected_data_length)) {
            fprintf(stderr, "ERROR in crypto_api_decode_ec_public_key: bad ECC point data octet string length!\n");
            return -1;
        }
        point_length = data_length;
        public_key_data->pXsize = point_length;
        memcpy(public_key_data->pX, der_data + data_offset, point_length);
    }

    return 0;
}

int crypto_api_encode_ec_signature(uint8_t * buffer, size_t buffer_size, uint32_t * signature_size, const EC_SIGNATURE_t * signature) {
    uint32_t r_integer_length = signature->rSize;
    uint32_t s_integer_length = signature->sSize;
    bool pad_r, pad_s;
    bool sequence_length_long_form;
    uint32_t expected_length = 0;
    uint32_t sequence_data_length = 0;
    uint32_t offset = 0;

    pad_r = false;
    if (0x80 & signature->r[0]) {
        r_integer_length++;
        pad_r = true;
    }
    pad_s = false;
    if (0x80 & signature->s[0]) {
        s_integer_length++;
        pad_s = true;
    }

    sequence_data_length += r_integer_length + 2; // INTEGER R
    sequence_data_length += s_integer_length + 2; // INTEGER S

    assert(sequence_data_length < 256);
    if (sequence_data_length >= 256) {
        return -1; // this should never happen
    }

    expected_length = 2;
    sequence_length_long_form = false;
    if (sequence_data_length > 127) {
        sequence_length_long_form = true;
        expected_length++;
    }
    expected_length += sequence_data_length;

    if (NULL != signature_size) {
        *signature_size = expected_length;
    }

    if (buffer_size < expected_length) {
        if (NULL == buffer) {
            return 0; // the caller just wants the expected size
        } else {
            return -1; // the buffer is too small!
        }
    }

    if (NULL == buffer) {
        return -1;
    }

    // set the sequence tag
    buffer[offset++] = IDENTIFIER_OCTET_SEQUENCE;
    // set the sequence length
    if (sequence_length_long_form) {
        // long form
        buffer[offset++] = 0x81;
        buffer[offset++] = (uint8_t)sequence_data_length;
    } else {
        // short form
        buffer[offset++] = (uint8_t)sequence_data_length;
    }
    // set the INTEGER R tag
    buffer[offset++] = IDENTIFIER_OCTET_INTEGER;
    buffer[offset++] = (uint8_t)r_integer_length;
    if (pad_r) {
        buffer[offset++] = 0;
    }
    memcpy(buffer + offset, signature->r, signature->rSize);
    offset += signature->rSize;
    // set the INTEGER S tag
    buffer[offset++] = IDENTIFIER_OCTET_INTEGER;
    buffer[offset++] = (uint8_t)s_integer_length;
    if (pad_s) {
        buffer[offset++] = 0;
    }
    memcpy(buffer + offset, signature->s, signature->sSize);
    offset += signature->sSize;

    assert(offset == expected_length);
    return 0;
}
