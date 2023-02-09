//
// Created by Molnar Botond on 2023. 02. 09..
//
#ifndef FLIGHT_COMPUTER_C_LORA_H
#define FLIGHT_COMPUTER_C_LORA_H
#include <stdio.h>
#include "sx127x.h"

#define LORA_SCK 18
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_SS 5
#define LORA_RST 0
#define LORA_DIO0 4

void lora_init();
void lora_tx_callback(sx127x *lora_device);
void lora_rx_callback(sx127x *lora_device);
void lora_handle_interrupt_task(void *arg);

#endif FLIGHT_COMPUTER_C_LORA_H
