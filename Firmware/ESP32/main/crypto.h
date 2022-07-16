#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>


#include "mbedtls/platform.h"
#include "mbedtls/entropy.h"
#include "mbedtls/md_internal.h"
#include "mbedtls/md.h"
#include "mbedtls/bignum.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/ecp_internal.h"
#include "mbedtls/pk.h"
#include "mbedtls/base64.h"

#ifndef CRYPTO_H_INCLUDED
#define CRYPTO_H_INCLUDED



/* include guards */

#define CURVE224 MBEDTLS_ECP_DP_SECP224R1

#define CURVE_KEY_SIZE 28
#define Simetric_Key_Size 32
#define AntiTrackingKey_size 72
#define Private_Key_Size CURVE_KEY_SIZE
#define Public_Key_Size CURVE_KEY_SIZE + 1 // 28+1 => 29
#define Uncompressed_Public_Key_Size 2*CURVE_KEY_SIZE + 1 // 2*28+1 => 57
#define Advertisement_Key_Size Public_Key_Size - 1 // 28



/* Prototypes for the functions */
/* Sums two ints */


int mbedtls_ansi_x936_kdf(mbedtls_md_type_t md_type, size_t input_len, uint8_t input[], size_t shared_info_len, uint8_t shared_info[], size_t output_len, uint8_t output[]);

void compressPublicKey(unsigned char* publicKey, unsigned char* compressedPublicKey);

void DeriveKeyPair(unsigned char publicKeyOutput[], unsigned char simetricKeyInputOutput[], unsigned char InitialPublicKey[]);


#endif