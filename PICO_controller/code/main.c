/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "lora.h"

void lora_gpio_callback(uint gpio, uint32_t events) {
    if (gpio == LORA_INTERRUPT_PIN) {
        int asd = 5;
    }
}

extern sx127x lora_device;

int main() {
    stdio_init_all();

    // Setup SPI
    spi_init(LORA_SPI_PORT, 1000000);

    gpio_set_function(LORA_MISO_PIN, GPIO_FUNC_SPI);
    gpio_set_function(LORA_MOSI_PIN, GPIO_FUNC_SPI);
    gpio_set_function(LORA_SCK_PIN, GPIO_FUNC_SPI);

    spi_set_slave(LORA_SPI_PORT, false);
    spi_set_format( spi0,   // SPI instance
                    8,      // Number of bits per transfer
                    1,      // Polarity (CPOL)
                    1,      // Phase (CPHA)
                    SPI_MSB_FIRST);

    // Setup CS pin
    gpio_init(LORA_NSS_PIN);
    gpio_set_dir(LORA_NSS_PIN, GPIO_OUT);
    gpio_put(LORA_NSS_PIN, 1);

    // Setup Lora interrupt pin
    gpio_set_dir(LORA_INTERRUPT_PIN, GPIO_IN);

    gpio_set_irq_enabled_with_callback(LORA_INTERRUPT_PIN, GPIO_IRQ_EDGE_RISE, true, &lora_gpio_callback);

    sx127x_create( &lora_device);
    int res = sx127x_set_opmod(SX127x_MODE_SLEEP, SX127x_MODULATION_LORA, &lora_device);
    sx127x_set_frequency(437200012, &lora_device);
    sx127x_lora_reset_fifo(&lora_device);
    sx127x_set_opmod(SX127x_MODE_STANDBY, SX127x_MODULATION_LORA, &lora_device);
    sx127x_lora_set_bandwidth(SX127x_BW_500000, &lora_device);
    sx127x_lora_set_implicit_header(NULL, &lora_device);
    sx127x_lora_set_modem_config_2(SX127x_SF_7, &lora_device);
    sx127x_lora_set_syncword(18, &lora_device);
    sx127x_set_preamble_length(8, &lora_device);

    sx127x_set_opmod(SX127x_MODE_RX_CONT, SX127x_MODULATION_LORA, &lora_device);
    sx127x_tx_set_pa_config(SX127x_PA_PIN_BOOST, 4, &lora_device);

    while (true) {

        sleep_ms(1);
    }
    return 0;
}
