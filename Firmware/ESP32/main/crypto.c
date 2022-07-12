#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>


#include "utils.h"
#include "crypto.h"

size_t dummy = 0;



int mbedtls_ansi_x936_kdf(mbedtls_md_type_t md_type, size_t input_len, uint8_t input[], size_t shared_info_len, uint8_t shared_info[], size_t output_len, uint8_t output[])
{
    mbedtls_md_context_t md_ctx;
    const mbedtls_md_info_t *md_info = NULL;
    int i = 0;
    int hashlen = 0;
    int exit_code = MBEDTLS_EXIT_FAILURE;
    uint32_t counter = 1;
    uint8_t counter_buf[4];

    mbedtls_md_init(&md_ctx);
    md_info = mbedtls_md_info_from_type(md_type);

    if (md_info == NULL)
    {
        mbedtls_fprintf(stderr, "Message Digest type not found\n");
        return (exit_code);
    }

    if (mbedtls_md_setup(&md_ctx, md_info, 0))
    {
        mbedtls_fprintf(stderr, "Failed to initialize context.\n");
        return (exit_code);
    }

    // TODO MAX HASH LENGTH FROM MBEDTLS replace pow sttement
    if (input_len + shared_info_len + 4 >= (pow(2, 61)) - 1)
    {
        mbedtls_fprintf(stderr, "Max hash length exceeded \n");
        return (exit_code);
    }

    // keydatalen equals output_len
    hashlen = md_info->size;
    uint8_t tmp_output[hashlen];
    if (output_len >= hashlen * (pow(2, 32) - 1))
    {
        mbedtls_fprintf(stderr, "Max hash length exceeded \n");
        return (exit_code);
    }

    while (i < output_len)
    {
        mbedtls_md_starts(&md_ctx);
        mbedtls_md_update(&md_ctx, input, input_len);

        // TODO: be careful with architecture little vs. big
        counter_buf[0] = (uint8_t)((counter >> 24) & 0xff);
        counter_buf[1] = (uint8_t)((counter >> 16) & 0xff);
        counter_buf[2] = (uint8_t)((counter >> 8) & 0xff);
        counter_buf[3] = (uint8_t)((counter >> 0) & 0xff);

        mbedtls_md_update(&md_ctx, counter_buf, 4);

        if (shared_info_len > 0 && shared_info != NULL)
        {
            mbedtls_md_update(&md_ctx, shared_info, shared_info_len);
        }
        mbedtls_md_finish(&md_ctx, tmp_output);
        memcpy(&output[i], tmp_output, (output_len - i < hashlen) ? output_len - i : hashlen);
        i += hashlen;
        counter++;
    }
    mbedtls_md_free(&md_ctx);
    return MBEDTLS_EXIT_SUCCESS;
}

void calculatePrivateKeyFromSharedData(unsigned char d_i[], unsigned char sharedData[], unsigned char privateKey[])
{
    int ret = 1;
    // load curve
    mbedtls_mpi order,
        u_i_bn,
        v_i_bn,
        d_0_bn,
        d_i_bn,
        tmp_bn;
    mbedtls_mpi_init(&order);
    mbedtls_mpi_init(&u_i_bn);
    mbedtls_mpi_init(&v_i_bn);
    mbedtls_mpi_init(&d_0_bn);
    mbedtls_mpi_init(&d_i_bn);
    mbedtls_mpi_init(&tmp_bn);

    mbedtls_ecp_group grp;
    mbedtls_ecp_group_init(&grp);
    mbedtls_ecp_group_load(&grp, CURVE224);

    // get order of (G) curve
    mbedtls_mpi_copy(&order, &grp.N);

    // (order of G) - 1
    mbedtls_mpi_sub_int(&order, &order, 1);

    size_t atKeys_size = AntiTrackingKey_size * sizeof(unsigned char) / 2; // Anti-Tracking Keys are half the size of the Shared Data

    unsigned char u_i_data[atKeys_size]; // firstHalf
    memset(u_i_data, '\0', atKeys_size);

    unsigned char v_i_data[atKeys_size]; // secondHalf
    memset(v_i_data, '\0', atKeys_size);

    memcpy(u_i_data, sharedData, atKeys_size);                         // copy first half of sharedData to u_i_data
    memcpy(v_i_data, sharedData + (AntiTrackingKey_size / 2), atKeys_size); // copy second half of sharedData to v_i_data

    MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary(&u_i_bn, u_i_data, atKeys_size)); // load u_i_bn from u_i_data
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary(&v_i_bn, v_i_data, atKeys_size)); // load v_i_bn from v_i_data

    // u_i_bn = u_i_bn % order + 1; // modulo order
    MBEDTLS_MPI_CHK(mbedtls_mpi_mod_mpi(&tmp_bn, &u_i_bn, &order));
    MBEDTLS_MPI_CHK(mbedtls_mpi_add_int(&tmp_bn, &tmp_bn, 1));
    MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&u_i_bn, &tmp_bn));

    // v_i_bn = v_i_bn % order + 1; // modulo order
    MBEDTLS_MPI_CHK(mbedtls_mpi_mod_mpi(&tmp_bn, &v_i_bn, &order));
    MBEDTLS_MPI_CHK(mbedtls_mpi_add_int(&tmp_bn, &tmp_bn, 1));
    MBEDTLS_MPI_CHK( mbedtls_mpi_copy(&v_i_bn, &tmp_bn));

    //  d_i_bn = (d_0_bn * u_i_bn) + v_i_bn;
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary(&d_0_bn, privateKey, Private_Key_Size)); // load d_0_bn from privateKey
    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&tmp_bn, &d_0_bn, &u_i_bn));                                 // tmp_bn = d_0_bn * u_i_bn
    MBEDTLS_MPI_CHK(mbedtls_mpi_add_mpi(&d_i_bn, &tmp_bn, &v_i_bn));                                 // d_i_bn = tmp_bn + v_i_bn

    // normalize d_i_bn to order
    MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&order, &grp.N));              // reset order of (G) curve
    MBEDTLS_MPI_CHK(mbedtls_mpi_mod_mpi(&d_i_bn, &d_i_bn, &order)); // d_i_bn = d_i_bn % order

    // return bytes of d_i_bn
    size_t d_i_bn_size = mbedtls_mpi_size(&d_i_bn);
    MBEDTLS_MPI_CHK(mbedtls_mpi_write_binary(&d_i_bn, d_i, d_i_bn_size));

    // unsigned char d_i_bn_data[d_i_bn_size];
    // mbedtls_mpi_write_binary( &d_i_bn, d_i_bn_data, d_i_bn_size );

    cleanup:
        mbedtls_mpi_free(&order);
        mbedtls_mpi_free(&u_i_bn);
        mbedtls_mpi_free(&v_i_bn);
        mbedtls_mpi_free(&d_0_bn);
        mbedtls_mpi_free(&d_i_bn);
        mbedtls_mpi_free(&tmp_bn);
        mbedtls_ecp_group_free(&grp);
}

void calculatePublicKeyFromPrivateKey(unsigned char publicKey[], unsigned char privateKey[])
{
    int ret = 1;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_pk_context pk;
    mbedtls_ecp_keypair *ec = malloc(sizeof(mbedtls_ecp_keypair));

    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);
    mbedtls_pk_init(&pk);
    mbedtls_ecp_keypair_init(ec);

    //set seed for random number generator
    MBEDTLS_MPI_CHK(mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *)"N/$!eW9zD6oA#X*/35b%", 20));

    mbedtls_ecp_group_id grp_id = CURVE224; // you need to convert this from the `TEE_ATTR_ECC_CURVE`
    MBEDTLS_MPI_CHK(mbedtls_pk_setup(&pk, mbedtls_pk_info_from_type(MBEDTLS_PK_ECDSA)));
    MBEDTLS_MPI_CHK(mbedtls_ecp_group_load(&ec->grp, grp_id));
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary(&ec->d, privateKey, Private_Key_Size));
    MBEDTLS_MPI_CHK(mbedtls_ecp_check_privkey(&ec->grp, &ec->d));
    MBEDTLS_MPI_CHK(mbedtls_ecp_mul(&ec->grp, &ec->Q, &ec->d, &ec->grp.G, mbedtls_ctr_drbg_random, &ctr_drbg));

    mbedtls_ecp_point_write_binary(&ec->grp, &ec->Q, MBEDTLS_ECP_PF_COMPRESSED, &dummy, publicKey, Public_Key_Size);

    cleanup:
        if (ret != 0)
        {
            mbedtls_printf("ERROR: Cleaning......\n");
        }
        mbedtls_ecp_keypair_free(ec);
        mbedtls_pk_free(&pk);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        mbedtls_entropy_free(&entropy);
}





void DeriveKeyPair(unsigned char publicKeyOutput[], unsigned char simetricKeyInputOutput[], unsigned char masterPrivateKey[])
{

    ///////////////////////////////////// (1) Derive Simetric Key ////////////////////////////////////////////////////
    unsigned char tmp_key[Simetric_Key_Size];
    // memset(tmp_key, '\0', Simetric_Key_Size);
    unsigned char update_shared[] = "update";
    mbedtls_ansi_x936_kdf(MBEDTLS_MD_SHA256, Simetric_Key_Size, simetricKeyInputOutput, strlen((const char *)update_shared), update_shared, Simetric_Key_Size, tmp_key);

    // save tmp_key -> simetricKeyInputOutput
    memcpy(simetricKeyInputOutput, tmp_key, Simetric_Key_Size);

    // mbedtls_printf("(1) DerivedSymetricKey: \n");
    // print_hex(simetricKeyInputOutput, Simetric_Key_Size, true);
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    ///////////////////////////////////// (2) Calculate Anti-Tracking Key ////////////////////////////////////////////
    unsigned char derivedAntiTrackingKeys[AntiTrackingKey_size];
    // memset(derivedAntiTrackingKeys, '\0', AntiTrackingKey_size);
    unsigned char diversify_info[] = "diversify";
    mbedtls_ansi_x936_kdf(MBEDTLS_MD_SHA256, Simetric_Key_Size, simetricKeyInputOutput, strlen((const char *)diversify_info), diversify_info, AntiTrackingKey_size, derivedAntiTrackingKeys);

    // mbedtls_printf("(2) DerivedAntiTrackingKeys: \n");
    // print_hex(derivedAntiTrackingKeys, AntiTrackingKey_size, true);
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    ///////////////////////////////////// (3) Calculate Private Key //////////////////////////////////////////////////
    // d_i = PrivateKey_i
    unsigned char d_i[Private_Key_Size];
    // memset(d_i, '\0', Private_Key_Size);
    calculatePrivateKeyFromSharedData(d_i, derivedAntiTrackingKeys, masterPrivateKey);

    // mbedtls_printf("(3) Calculate Private Key: \n");
    // print_hex(d_i, Private_Key_Size, true);
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    ///////////////////////////////////// (4) Calculate Public Key //////////////////////////////////////////////////
    // unsigned char derivedPublicKey[Public_Key_Size];
    // memset(derivedPublicKey, '\0', Public_Key_Size);
    calculatePublicKeyFromPrivateKey(publicKeyOutput, d_i);

    // mbedtls_printf("(4) DerivedPublicKey: \n");
    // print_hex(publicKeyOutput, Public_Key_Size, true);
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////

}
