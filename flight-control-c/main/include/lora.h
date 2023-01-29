#ifndef LORA_H
#define LORA_H

#define LORA_SCK_PIN 18
#define LORA_MISO_PIN 19
#define LORA_MOSI_PIN 23
#define LORA_SS_PIN 5
#define LORA_RST_PIN 0
#define LORA_DIO0_PIN 4



void add_lora_spi_interface(void);
void init_lora();


#endif