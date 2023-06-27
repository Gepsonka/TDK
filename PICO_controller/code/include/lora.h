#ifndef LORA_H
#define LORA_H

// // #include <Arduino.h>
// #include <SPI.h>

#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "string.h"
#include "crc.h"



#define LORA_SPI_PORT spi0
#define LORA_MISO_PIN 16
#define LORA_MOSI_PIN 19
#define LORA_SCK_PIN 18
#define LORA_NSS_PIN 17
#define LORA_RESET_PIN 20
#define LORA_INTERRUPT_PIN 22

#define PA_OUTPUT_RFO_PIN          0
#define PA_OUTPUT_PA_BOOST_PIN     1

#define LORA_PAYLOAD_MAX_SIZE 255
#define LORA_HEADER_SIZE 7

typedef enum {
    NETWORK_OK = 0,
    NETWORK_INVALID_PACKET_SIZE = -1,
    NETWORK_INVALID_HEADER_CRC = -2,
    NETWORK_INVALID_PAYLOAD_CRC = -3
} LoRa_Network_Status;

typedef struct {
    uint8_t src_device_addr;
    uint8_t dest_device_addr;
    uint8_t num_of_packets;
    uint8_t packet_num;
    uint8_t payload_size;
    uint16_t header_crc;
} LoRa_Packet_Header;

typedef struct {
    uint8_t payload[LORA_PAYLOAD_MAX_SIZE]; // Lora packet at 128 coding rate is 256 bytes - 7 bytes of header - 2 bytes crc
    uint16_t payload_crc;
} LoRa_Packet_Payload;

typedef struct  {
    LoRa_Packet_Header header;
    LoRa_Packet_Payload payload;

} LoRa_Packet;


int8_t lora_parse_packet(LoRa_Packet* lora_packet, const uint8_t* buff, size_t packet_size);


// Low level functionality

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


int begin(LoRa* lora_dev, long frequency);
void end();

int beginPacket(LoRa* lora_dev, int implicitHeader);
int endPacket(LoRa* lora_dev, bool async);

int parsePacket(LoRa* lora_dev, int size);
int packetRssi(LoRa* lora_dev);
float packetSnr();
long packetFrequencyError(LoRa* lora_dev);

int rssi(LoRa* lora_dev);

void onReceive(LoRa* lora_dev, void(*callback)(LoRa*, int));
void onTxDone(LoRa* lora_dev, void(*callback)(LoRa*));

void receive(LoRa* lora_dev, int size);

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
void enableInvertIQ();
void disableInvertIQ();

void setOCP(uint8_t mA); // Over Current Protection control

void setGain(uint8_t gain); // Set LNA gain

uint8_t random();

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

uint8_t readRegister( uint8_t address);
void writeRegister( uint8_t address, uint8_t value);
uint8_t singleTransfer( uint8_t address, uint8_t value);

static void onDio0Rise( uint, uint32_t);

int lora_rx_read_payload(LoRa* lora_dev, uint8_t* buffer, uint8_t packet_size);
int lora_set_syncword(uint8_t value);
int lora_set_preamble_length(uint16_t value);


#endif