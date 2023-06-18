/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "lora.h"
#include "servo.h"
#include "elevon.h"
#include "motor.h"



extern LoRa lora_device;

void lora_on_receive(LoRa* lora_dev, int packet_size) {
    uint8_t buff[255];
    uint8_t size_packet;

    lora_rx_read_payload(buff, &size_packet);
    for (int i = 0; i < packet_size; i++) {
        buff[i] = read(lora_dev);
    }
    int adsd;

}




int main() {
    stdio_init_all();

    gpio_init(LORA_INTERRUPT_PIN);
    gpio_set_dir(LORA_INTERRUPT_PIN, GPIO_IN);
    gpio_pull_down(LORA_INTERRUPT_PIN);


    begin(&lora_device, 437200012);
    onReceive(&lora_device, &lora_on_receive);
    disableCrc();
    setSignalBandwidth(500E3);
    explicitHeaderMode(&lora_device);
    setCodingRate4(0);

    receive(&lora_device, -1);

    elevon_init();
    motor_init();

    while (true) {

        sleep_ms(1);
    }
    return 0;
}
