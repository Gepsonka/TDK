#ifndef LORA_H
#define LORA_H
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "sx127x.h"
#include <sx127x.h>
#include <stdlib.h>
#include <driver/spi_common.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include "esp_crc.h"
#include <rom/crc.h>
#include "memory.h"

#define LORA_SPI_HOST VSPI_HOST

#define LORA_SCK_PIN 18
#define LORA_MISO_PIN 19
#define LORA_MOSI_PIN 23
#define LORA_SS_PIN 5
#define LORA_RST_PIN 0
#define LORA_DIO0_PIN 4

#define LORA_PAYLOAD_MAX_SIZE 246
#define LORA_MAX_NUM_OF_PACKETS 10 // Will be changed, currently this is enough

#define LORA_BASE_STATION_ADDR 0x00
#define LORA_NETWORK_BROADCAST_ADDR 0xFF

// 1 byte addr space allows 255 device in the network
// currently it does the job, and perfect for the use case
#define BASE_STATION_ADDR 0X00 // TODO later: DHCP server impl
#define BROADCAST_ADDR 0XFF

typedef struct {
    uint8_t src_device_addr;
    uint8_t dest_device_addr;
    uint8_t num_of_packets;
    uint8_t packet_num;
    uint8_t payload_size;
    uint16_t header_crc;
} LoRa_Packet_Header;

typedef struct {
    uint8_t payload[LORA_PAYLOAD_MAX_SIZE]; // Lora packet at 128 coding rate is 256 bytes - 7 bytes of header - 2 bytes crc
    uint16_t payload_crc;
} LoRa_Packet_Payload;

typedef struct  {
    LoRa_Packet_Header header;
    LoRa_Packet_Payload payload;

} LoRa_Packet;

void IRAM_ATTR lora_handle_interrupt_fromisr(void *arg);

void init_lora();

void handle_interrupt_task(void *arg);
void tx_callback(sx127x *device);
void rx_callback(sx127x *device);

void lora_packet_sender_task(void* pvParameters);
uint8_t lora_send_packet(sx127x *lora_device, LoRa_Packet* packet);


/// Returned when message is fragmented and sent
typedef enum {
    MESSAGE_OK = 0x00,
    MESSAGE_NOT_ENOUGH_MEMORY = 0x02
} Message_Process_Status;

///Fragments message into packages, inits packages, calculate 16 bit CRC, then sends it to the
/// message sender task.
/// \param src_addr Source network address.
/// \param dest_addr Destination network address.
/// \param message Message to send in a uint8_t array
/// \param message_len Length of the message
/// \return 0 if successful, anything else is error.
uint8_t lora_send_message(uint8_t src_addr, uint8_t dest_addr, uint8_t* message, uint8_t message_len);

/// The network is pinged by the base station at every 2 seconds,
/// so other devices get alerted and with the correct
/// password can join to the network.
/// \param pvParameters Task parameter
void lora_pinging_task(void* pvParameters);


#endif