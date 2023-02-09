#include "lora.h"
#include <sx127x.h>
#include <stdlib.h>
#include <driver/spi_common.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>



sx127x *lora_device = NULL;
spi_device_handle_t lora_spi_device;
uint8_t lora_rx_buff[2048];
uint8_t lora_tx_buff[2048];


typedef struct  {
    uint16_t src_device_addr;
    uint16_t dest_device_addr;
    uint8_t num_of_packets;
    uint8_t pakcet_num;
    uint8_t payload[514];
} LoRa_Packet;