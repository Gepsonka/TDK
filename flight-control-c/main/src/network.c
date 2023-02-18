//
// Created by molnar on 2023.02.14..
//
#include "network.h"

Network_Device_Container device_container;

void network_init_device_container(Network_Device_Container* device_cont)
{
    device_cont->device_contexts = NULL;
    device_cont->num_of_devices = 0;
}

network_operation_t network_add_device(Network_Device_Container* device_cont, uint8_t dev_addr)
{
    Network_Device_Context new_device;
    new_device.address = dev_addr;
    new_device.cipher_text = NULL;
    new_device.tx_secret_message = NULL;
    new_device.tx_secret_message_size = 0;
    new_device.rx_secret_message = NULL;
    new_device.rx_secret_message_size = 0;
    new_device.tx_message = NULL;
    new_device.tx_message_size = 0;
    new_device.rx_message = NULL;
    new_device.rx_message_size = 0;
    new_device.network_rx_buff = NULL;
    new_device.network_tx_buff = NULL;

    if (device_cont->num_of_devices == 0) {
        device_cont->num_of_devices++;
        device_cont->device_contexts = (Network_Device_Context*) malloc(sizeof(Network_Device_Context));

    } else {
        device_cont->num_of_devices++;
        device_cont->device_contexts = (Network_Device_Context*) realloc(device_cont->device_contexts, device_cont->num_of_devices * sizeof(Network_Device_Context));
    }

    if (device_cont->device_contexts == NULL) {
        return NETWORK_OUT_OF_MEMORY;
    }

    device_cont->device_contexts[device_cont->num_of_devices - 1] = new_device;
    return NETWORK_OK;
}

void network_generate_security_credentials_for_device(Network_Device_Context* device_ctx){
    esp_fill_random(device_ctx->init_vector, SECURITY_INIT_VECTOR_SIZE);
    esp_fill_random(device_ctx->aes_key, SECURITY_AES_KEY_SIZE_BYTE);
    esp_fill_random(device_ctx->aad, SECURITY_ADDITIONAL_AUTH_DATA_SIZE);
}

void network_encrypt_device_message(Network_Device_Context* device_ctx){
    if (device_ctx->tx_secret_message != NULL) {
        free(device_ctx->tx_secret_message);
        device_ctx->tx_secret_message_size = 0;
    }

    device_ctx->tx_secret_message = (uint8_t*) malloc(device_ctx->tx_message_size * sizeof(uint8_t));
    device_ctx->tx_secret_message_size = device_ctx->tx_message_size;

    aes_gcm_encrypt(
                device_ctx->aes_key,
                device_ctx->init_vector,
                device_ctx->tx_message,
                device_ctx->tx_message_size,
                device_ctx->aad,
                device_ctx->tx_secret_message,
                device_ctx->tx_auth_tag
            );
}


network_operation_t network_decrypt_device_message(Network_Device_Context* device_ctx){
    uint8_t decrypted_auth_tag[SECURITY_AUTH_TAG_SIZE];

    if (device_ctx->rx_message != NULL) {
        free(device_ctx->rx_message);
        device_ctx->rx_message_size = 0;
    }

    // Put received auth tag into rx_auth_tag
    network_get_auth_tag_from_secret_message(device_ctx);


    device_ctx->rx_message = (uint8_t*) malloc(device_ctx->rx_secret_message_size * sizeof(uint8_t));
    device_ctx->rx_message_size = device_ctx->rx_secret_message_size;

    aes_gcm_decrypt(
            device_ctx->aes_key,
            device_ctx->init_vector,
            device_ctx->tx_message,
            device_ctx->aad,
            device_ctx->rx_secret_message,
            device_ctx->rx_secret_message_size,
            decrypted_auth_tag
    );

    for (uint8_t i = 0; i < SECURITY_AUTH_TAG_SIZE; i++){
        if (decrypted_auth_tag[i] != device_ctx->rx_auth_tag[i]) {
            return NETWORK_UNAUTHENTICATED;
        }
    }

    return NETWORK_OK;
}


void network_get_auth_tag_from_secret_message(Network_Device_Context* device_ctx){
    memcpy(device_ctx->rx_auth_tag, &(device_ctx->rx_secret_message[0]), SECURITY_AUTH_TAG_SIZE);
    uint8_t* naked_message_buffer = (uint8_t*) malloc((device_ctx->rx_secret_message_size - SECURITY_AUTH_TAG_SIZE) * sizeof(uint8_t));
    memcpy(&(device_ctx->rx_secret_message[SECURITY_AUTH_TAG_SIZE + 1]), naked_message_buffer, device_ctx->rx_secret_message_size - SECURITY_AUTH_TAG_SIZE);

    free(naked_message_buffer);
}
