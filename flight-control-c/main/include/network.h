//
// Created by molnar on 2023.02.14..
//

#ifndef FLIGHT_COMPUTER_NETWORK_H
#define FLIGHT_COMPUTER_NETWORK_H

#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "security.h"

typedef enum {
    ONLINE,
    OFFLINE,
    KEY_EXCHANGE_STARTED,
} Network_Device_Status;

typedef struct {
    uint8_t address;
    Network_Device_Status status;
    uint8_t init_vector[SECURITY_INIT_VECTOR_SIZE];
    uint8_t aes_key[SECURITY_INIT_VECTOR_SIZE];
    uint8_t auth_tag[SECURITY_AUTH_TAG_SIZE];
    uint8_t* cipher_text;
    uint8_t* tx_message; // to be encrypted
    uint16_t tx_message_size;
    uint8_t* rx_message; // to be decrypted
    uint16_t rx_message_size;
    uint8_t* receive_buff;
    uint8_t* transmit_buff;
} Network_Device_Context;

typedef struct {
    Network_Device_Context* device_contexts;
    uint8_t num_of_devices;
} Network_Device_Container;


void network_add_device(Network_Device_Container* device_cont, uint8_t dev_addr);
void network_encrypt_device_message(Network_Device_Context* device_ctx);

/// Free up dynamic device buffers after transmission.
/// \param device_ctx device context
void network_free_device_ctx(Network_Device_Context* device_ctx);

#endif //FLIGHT_COMPUTER_NETWORK_H
