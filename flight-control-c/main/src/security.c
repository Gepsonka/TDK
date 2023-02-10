//
// Created by molnar on 2023.02.09..
//

#include "security.h"

uint8_t aes_key[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

uint8_t plain_data[] = {
        0x55, 0x73, 0x69, 0x6e, 0x67, 0x20, 0x6c, 0x69,
        0x62, 0x69, 0x63, 0x61, 0x20, 0x69, 0x73, 0x20,
        0x73, 0x6d, 0x61, 0x72, 0x74, 0x20, 0x61, 0x6e,
        0x64, 0x20, 0x65, 0x61, 0x73, 0x79, 0x21, 0x00
};

uint8_t iv[12] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B
};

mbedtls_gcm_context aes;

void aes_gcm(mbedtls_gcm_context* aes_ctx,
             uint8_t* aes_key,
             uint8_t operation,
             uint8_t *init_vec,
             uint8_t len_of_init_vec,
             uint8_t* input,
             uint16_t input_len,
             uint8_t * output,
             uint16_t output_length_in_bytes,
             uint16_t output_length
             ){
    mbedtls_gcm_init(aes_ctx);
    printf("AES context created\n");
    mbedtls_gcm_setkey(aes_ctx,MBEDTLS_CIPHER_ID_AES , aes_key, 16 * 8);
    printf("Setting AES key\n");
    mbedtls_gcm_starts(aes_ctx, operation, init_vec, len_of_init_vec);
    printf("Send the initialized cipher some data and store it...\n");
    mbedtls_gcm_update(aes_ctx, input, input_len, output, output_length_in_bytes, (size_t *) output_length);
    printf("Write encrypred text to output\n");
    mbedtls_gcm_free(aes_ctx);
    printf("Free ctx\n  ");
}