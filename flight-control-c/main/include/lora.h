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
#include <esp_crc.h>
#include "memory.h"

#define LORA_SPI_HOST VSPI_HOST

#define LORA_SCK_PIN 18
#define LORA_MISO_PIN 19
#define LORA_MOSI_PIN 23
#define LORA_SS_PIN 5
#define LORA_RST_PIN 0
#define LORA_DIO0_PIN 4

// 1 byte addr space allows 255 device in the network
// currently it does the job, and perfect for the use case
#define BASE_STATION_ADDR 0X00 // TODO later: DHCP server impl
#define BROADCAST_ADDR 0XFF


typedef struct  {
    uint8_t src_device_addr;
    uint8_t dest_device_addr;
    uint8_t num_of_packets;
    uint8_t packet_num;
    uint8_t payload_size;
    uint16_t header_crc;
    uint8_t payload[245]; // Lora packet at 128 coding rate is 255 byte - 8 bytes of header
    uint16_t payload_crc;
} LoRa_Packet;

void IRAM_ATTR lora_handle_interrupt_fromisr(void *arg);

void init_lora();

void handle_interrupt_task(void *arg);
void tx_callback(sx127x *device);
void rx_callback(sx127x *device);

uint8_t lora_send_packet(sx127x *lora_device, LoRa_Packet* packet);
uint8_t lora_fragment_message();



#endif