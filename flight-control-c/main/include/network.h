//
// Created by molnar on 2023.02.14..
//

#ifndef FLIGHT_COMPUTER_NETWORK_H
#define FLIGHT_COMPUTER_NETWORK_H

#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "security.h"
#include "lora.h"

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
    uint8_t* tx_secret_message; // to be encrypted
    uint16_t tx_secret_message_size;
    uint8_t* rx_secret_message; // to be decrypted
    uint16_t rx_secret_message_size;
    LoRa_Packet* network_tx_buff; // assembled packets to be sent to device
    LoRa_Packet* network_rx_buff; // stores last received packets from device
} Network_Device_Context;

typedef struct {
    Network_Device_Context* device_contexts;
    uint8_t num_of_devices;
} Network_Device_Container;


void network_add_device(Network_Device_Container* device_cont, uint8_t dev_addr);
void network_encrypt_device_message(Network_Device_Context* device_ctx);
void network_decrypt_device_message(Network_Device_Context* device_ctx);

/// Free up device dynamic buffers.
/// \param device_ctx device context
void network_free_device_ctx(Network_Device_Context* device_ctx);
void network_free_device_cipher_txt(Network_Device_Context* device_ctx);
void network_free_device_tx_secret_message(Network_Device_Context* device_ctx);
void network_free_device_rx_secret_message(Network_Device_Context* device_ctx);
void network_free_device_network_rx_buff(Network_Device_Context* device_ctx);
void network_free_device_network_tx_buff(Network_Device_Context* device_ctx);


#endif //FLIGHT_COMPUTER_NETWORK_H
