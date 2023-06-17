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

typedef struct LoRa LoRa;

typedef struct LoRa{
    spi_inst_t* spi;
    uint8_t nss_pin;
    uint8_t reset_pin;
    uint8_t interrupt_pin;
    long frequency;
    uint packet_index;
    uint8_t implicit_header;
    void (*onReceive)(LoRa*, int);
    void (*onTxDone)(LoRa*);
};

uint8_t lora_init(

        uint8_t nss_pin,
        uint8_t
        );

int begin(LoRa* lora_dev, long frequency);
void end();

int beginPacket(int implicitHeader);
int endPacket(LoRa* lora_dev, bool async);

int parsePacket(LoRa* lora_dev, int size);
int packetRssi(LoRa* lora_dev);
float packetSnr();
long packetFrequencyError(LoRa* lora_dev);

int rssi(LoRa* lora_dev);

void onReceive(LoRa* lora_dev, void(*callback)(LoRa*, int));
void onTxDone(LoRa* lora_dev, void(*callback)(LoRa*));

void receive(int size);

void idle();
void sleep();

size_t write(const uint8_t *buffer, size_t size);
int read(LoRa* lora_dev);
int peek(LoRa* lora_dev);

// size_t print(const char* c);

void setTxPower(int level, int outputPin);
void setFrequency(LoRa* lora_dev, long frequency);
void setSpreadingFactor(int sf);
void setSignalBandwidth(long sbw);
void setCodingRate4(int denominator);
void setPreambleLength(long length);
void setSyncWord(int sw);
void enableCrc();
void disableCrc();
void enableInvertIQ(LoRa* lora_dev);
void disableInvertIQ(LoRa* lora_dev);

void setOCP(LoRa* lora_dev, uint8_t mA); // Over Current Protection control

void setGain(LoRa* lora_dev, uint8_t gain); // Set LNA gain

// deprecated
void crc() { enableCrc(); }
void noCrc() { disableCrc(); }

uint8_t random(LoRa* lora_dev);

void setPins(LoRa* lora_dev, int ss, int reset, int dio0);
void setSPI(LoRa* lora_dev, spi_inst_t* spi);
void setSPIFrequency(uint32_t frequency);

void dumpRegisters();

void explicitHeaderMode(LoRa* lora_dev);
void implicitHeaderMode(LoRa* lora_dev);

void handleDio0Rise(LoRa* lora_dev);
bool isTransmitting();

int getSpreadingFactor();
long getSignalBandwidth();

void setLdoFlag();

uint8_t readRegister(LoRa* lora_dev, uint8_t address);
void writeRegister(LoRa* lora_dev, uint8_t address, uint8_t value);
uint8_t singleTransfer(LoRa* lora_dev, uint8_t address, uint8_t value);

static void onDio0Rise(LoRa* lora_dev, uint, uint32_t);

#endif