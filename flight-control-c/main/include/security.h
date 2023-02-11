//
// Created by molnar on 2023.02.09..
//
#ifndef SECURITY_H
#define SECURITY_H
#include <stdio.h>
#include "string.h"
#include "mbedtls/gcm.h"

#define AES_GCM_ENCRYPT MBEDTLS_GCM_ENCRYPT
#define AES_GCM_DECRYPT MBEDTLS_GCM_DECRYPT

#define AES_KEY_SIZE_BIT 128
#define INIT_VECTOR_LENGTH 12

void aes_gcm(mbedtls_gcm_context* aes_ctx,
             uint8_t* aes_key,
             uint8_t operation,
             uint8_t *init_vec,
             uint8_t len_of_init_vec,
             uint8_t* input,
             uint16_t input_len,
             uint8_t * output,
             uint16_t output_length_in_bytes,
             size_t* output_length
             );


#endif SECURITY_H
