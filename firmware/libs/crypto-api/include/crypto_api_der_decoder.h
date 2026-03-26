#ifndef __CRYPTO_API_DER_DECODER_H__
#define __CRYPTO_API_DER_DECODER_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "crypto_api.h"

int crypto_api_decode_ec_public_key(const uint8_t * der_data, uint32_t der_data_size, PUBLIC_KEY_EC_t * public_key_data);

int crypto_api_encode_ec_signature(uint8_t * buffer, size_t buffer_size, uint32_t * signature_size, const EC_SIGNATURE_t * signature);

#endif
