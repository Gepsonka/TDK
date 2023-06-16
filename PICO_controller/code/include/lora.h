#ifndef LORA_H
#define LORA_H

// // #include <Arduino.h>
// #include <SPI.h>

#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "string.h"

#define PIN_MISO 16
#define PIN_CS   8
#define PIN_SCK  18
#define PIN_MOSI 19

#define SPI_PORT spi0
#define READ_BIT 0x80

#define LORA_DEFAULT_SPI           spi0
#define LORA_DEFAULT_SPI_FREQUENCY 8E6
#define LORA_DEFAULT_SS_PIN        8
#define LORA_DEFAULT_RESET_PIN     9
#define LORA_DEFAULT_DIO0_PIN      7

#define LORA_SPI_PORT spi0

#define LORA_MISO_PIN 16
#define LORA_MOSI_PIN 19
#define LORA_SCK_PIN 18
#define LORA_NSS_PIN 17
#define LORA_RESET_PIN 20
#define LORA_INTERRUPT_PIN 22

#define PA_OUTPUT_RFO_PIN          0
#define PA_OUTPUT_PA_BOOST_PIN     1




typedef struct LoRa {
    uint8_t nss_pin;
    uint8_t reset_pin;
    uint8_t interrupt_pin;
    long frequency;
    uint packet_index;
    uint8_t implicit_header;
    void (*onReceive)(int);
    void (*onTxDone)();
};

uint8_t lora_init(

        uint8_t nss_pin,
        uint8_t
        );

int begin(long frequency);
void end();

int beginPacket(int implicitHeader);
int endPacket(bool async);

int parsePacket(int size);
int packetRssi();
float packetSnr();
long packetFrequencyError();

int rssi();

void onReceive(void(*callback)(int));
void onTxDone(void(*callback)());

void receive(int size);

void idle();
void sleep();

// size_t print(const char* c);

void setTxPower(int level, int outputPin);
void setFrequency(long frequency);
void setSpreadingFactor(int sf);
void setSignalBandwidth(long sbw);
void setCodingRate4(int denominator);
void setPreambleLength(long length);
void setSyncWord(int sw);
void enableCrc();
void disableCrc();
void enableInvertIQ();
void disableInvertIQ();

void setOCP(uint8_t mA); // Over Current Protection control

void setGain(uint8_t gain); // Set LNA gain

// deprecated
void crc() { enableCrc(); }
void noCrc() { disableCrc(); }

uint8_t random();

void setPins(int ss, int reset, int dio0);
void setSPI(spi_inst_t* spi);
void setSPIFrequency(uint32_t frequency);

void dumpRegisters();

void explicitHeaderMode();
void implicitHeaderMode();

void handleDio0Rise();
bool isTransmitting();

int getSpreadingFactor();
long getSignalBandwidth();

void setLdoFlag();

uint8_t readRegister(uint8_t address);
void writeRegister(uint8_t address, uint8_t value);
uint8_t singleTransfer(uint8_t address, uint8_t value);

static void onDio0Rise(uint, uint32_t);

#endif