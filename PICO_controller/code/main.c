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

int8_t x_dir_percentage;
int8_t y_dir_percentage;
int8_t z_dir_percentage;
uint8_t thr_percentage;



void lora_on_receive(LoRa* lora_dev, int packet_size) {
    uint8_t buff[255];
    size_t size_packet;
    LoRa_Packet lora_packet;

    //lora_rx_read_payload(&lora_device, buff, packet_size);

    for (uint8_t i = 0; i < packet_size; i++) {
        buff[i] = read(lora_dev);
    }

    const int8_t parse_result = lora_parse_packet(&lora_packet, buff, packet_size);
    memset(&x_dir_percentage, buff[9], sizeof(int8_t));
    memset(&y_dir_percentage, buff[10], sizeof(int8_t));
    memset(&z_dir_percentage, buff[11], sizeof(int8_t));
    thr_percentage = (uint8_t) buff[12];


    float left_e_mix = elevon_left_elevon_mix(x_dir_percentage, y_dir_percentage, -0.5f);
    float right_e_mix = elevon_right_elevon_mix(x_dir_percentage, y_dir_percentage, -0.5f);

    elevon_set_elevon_by_percentage(LEFT_ELEVON, left_e_mix);
    elevon_set_elevon_by_percentage(RIGHT_ELEVON, right_e_mix);

    int csoki = 1;
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
    setSpreadingFactor(7);
    setPreambleLength(8);
    //lora_set_syncword(8);
//    lora_set_preamble_length(18);


    receive(&lora_device, -1);

    elevon_init();
    motor_init();

    while (true) {

        sleep_ms(1);
    }
    return 0;
}
