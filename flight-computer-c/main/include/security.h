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

#define SECURITY_AES_KEY_SIZE_BIT 128
#define SECURITY_AES_KEY_SIZE_BYTE 16
#define SECURITY_INIT_VECTOR_SIZE 12 // minimum for strong security
#define SECURITY_AUTH_TAG_SIZE 16 // Strong enough
#define SECURITY_ADDITIONAL_AUTH_DATA_SIZE 16

void aes_gcm_encrypt(
        uint8_t* key,
        uint8_t* init_vec,
        uint8_t* plain_data,
        uint8_t plain_data_len,
        uint8_t* aad_data,
        uint8_t* ciphertext,
        uint8_t* tag
);

void aes_gcm_decrypt(
        uint8_t* key,
        uint8_t* init_vec,
        uint8_t* plain_data,
        uint8_t* aad_data,
        uint8_t* ciphertext,
        uint8_t ciphertext_len,
        uint8_t* tag
);

#endif
