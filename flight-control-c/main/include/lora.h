#ifndef LORA_H
#define LORA_H
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sx127x.h"
#include <sx127x.h>
#include <stdlib.h>
#include <driver/spi_common.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <esp_log.h>



#define LORA_SPI_HOST VSPI_HOST

#define LORA_SCK_PIN 18
#define LORA_MISO_PIN 19
#define LORA_MOSI_PIN 23
#define LORA_SS_PIN 5
#define LORA_RST_PIN 0
#define LORA_DIO0_PIN 4

typedef struct  {
    uint8_t src_device_addr;
    uint8_t dest_device_addr;
    uint8_t num_of_packets;
    uint8_t packet_num;
    uint8_t payload[250];
    uint16_t crc;
} LoRa_Packet_Header;

void IRAM_ATTR lora_handle_interrupt_fromisr(void *arg);

void init_lora();

void handle_interrupt_task(void *arg);
void tx_callback(sx127x *device);
void rx_callback(sx127x *device);



#endif