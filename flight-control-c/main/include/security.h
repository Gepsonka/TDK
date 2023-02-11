//
// Created by molnar on 2023.02.09..
//
#ifndef SECURITY_H
#define SECURITY_H
#include <stdio.h>
#include "string.h"
#include "mbedtls/gcm.h"
#include "mbedtls/pk.h"

#define AES_GCM_ENCRYPT MBEDTLS_GCM_ENCRYPT
#define AES_GCM_DECRYPT MBEDTLS_GCM_DECRYPT

#define AES_KEY_SIZE_BIT 128
#define INIT_VECTOR_LENGTH 12

void aes_gcm_encrypt(mbedtls_gcm_context* aes_ctx,
             uint8_t* key,
             uint8_t* init_vec,
             uint8_t init_vec_len,
             uint8_t* plain_data,
             uint8_t plain_data_len,
             uint8_t* add_data,
             uint8_t add_data_len,
             uint8_t* ciphertext,
             uint8_t* tag,
             uint8_t tag_len
             );

void aes_gcm_decrypt(mbedtls_gcm_context* aes_ctx,
                     uint8_t* key,
                     uint8_t* init_vec,
                     uint8_t init_vec_len,
                     uint8_t* plain_data,
                     uint8_t* aad_data,
                     uint8_t aad_data_len,
                     uint8_t* ciphertext,
                     uint8_t ciphertext_len,
                     uint8_t* tag,
                     uint8_t tag_len
);

#endif
