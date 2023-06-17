/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "lora.h"

extern LoRa lora_device;

void lora_on_receive(LoRa* lora_dev, int pakcet_size) {
    uint8_t asd = read(&lora_device);
    int csoki = asd;
}

void lora_rx_interrupt() {
    int asd = read(&lora_device);

}


int main() {
    stdio_init_all();

    gpio_init(LORA_INTERRUPT_PIN);
    gpio_set_dir(LORA_INTERRUPT_PIN, GPIO_IN);
    gpio_pull_down(LORA_INTERRUPT_PIN);


    int8_t res = begin(&lora_device, 437200012);
    onReceive(&lora_device, &lora_on_receive);
    receive(&lora_device, 1);

    while (true) {

        sleep_ms(1);
    }
    return 0;
}
