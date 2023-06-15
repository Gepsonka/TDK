//
// Created by gepsonka on 6/15/23.
//

#ifndef PICO_CONTROLLER_LORA_H
#define PICO_CONTROLLER_LORA_H

#include "pico/stdlib.h"
#include "hardware/spi.h"

// registers
#define REG_FIFO                 0x00
#define REG_OP_MODE              0x01
#define REG_FRF_MSB              0x06
#define REG_FRF_MID              0x07
#define REG_FRF_LSB              0x08
#define REG_PA_CONFIG            0x09
#define REG_OCP                  0x0b
#define REG_LNA                  0x0c
#define REG_FIFO_ADDR_PTR        0x0d
#define REG_FIFO_TX_BASE_ADDR    0x0e
#define REG_FIFO_RX_BASE_ADDR    0x0f
#define REG_FIFO_RX_CURRENT_ADDR 0x10
#define REG_IRQ_FLAGS            0x12
#define REG_RX_NB_BYTES          0x13
#define REG_PKT_SNR_VALUE        0x19
#define REG_PKT_RSSI_VALUE       0x1a
#define REG_RSSI_VALUE           0x1b
#define REG_MODEM_CONFIG_1       0x1d
#define REG_MODEM_CONFIG_2       0x1e
#define REG_PREAMBLE_MSB         0x20
#define REG_PREAMBLE_LSB         0x21
#define REG_PAYLOAD_LENGTH       0x22
#define REG_MODEM_CONFIG_3       0x26
#define REG_FREQ_ERROR_MSB       0x28
#define REG_FREQ_ERROR_MID       0x29
#define REG_FREQ_ERROR_LSB       0x2a
#define REG_RSSI_WIDEBAND        0x2c
#define REG_DETECTION_OPTIMIZE   0x31
#define REG_INVERTIQ             0x33
#define REG_DETECTION_THRESHOLD  0x37
#define REG_SYNC_WORD            0x39
#define REG_INVERTIQ2            0x3b
#define REG_DIO_MAPPING_1        0x40
#define REG_VERSION              0x42
#define REG_PA_DAC               0x4d

// modes
#define MODE_LONG_RANGE_MODE     0x80
#define MODE_SLEEP               0x00
#define MODE_STDBY               0x01
#define MODE_TX                  0x03
#define MODE_RX_CONTINUOUS       0x05
#define MODE_RX_SINGLE           0x06

// PA config
#define PA_BOOST                 0x80

// IRQ masks
#define IRQ_TX_DONE_MASK           0x08
#define IRQ_PAYLOAD_CRC_ERROR_MASK 0x20
#define IRQ_RX_DONE_MASK           0x40

#define RF_MID_BAND_THRESHOLD    525E6
#define RSSI_OFFSET_HF_PORT      157
#define RSSI_OFFSET_LF_PORT      164

#define MAX_PKT_LENGTH           255

#define LORA_SPI_PORT 0

#define LORA_MISO_PIN 16
#define LORA_MOSI_PIN 19
#define LORA_SCK_PIN 18
#define LORA_NSS_PIN 17
#define LORA_RESET_PIN 20


typedef struct {
    
};



int begin(long frequency);
void end();
int beginPacket(int implicitHeader);
int endPacket(bool async);
bool isTransmitting();
int parsePacket(int size);
int packetRssi();
float packetSnr();
long packetFrequencyError();
int rssi();
size_t write(uint8_t byte);
int available();
int read();
int peek();
void flush();
void onTxDone(void(*callback)());
void receive(int size);
void idle();
void sleep();
void setTxPower(int level, int outputPin);
void setFrequency(long frequency);
int getSpreadingFactor();
void setSpreadingFactor(int sf);
long getSignalBandwidth();
void setSignalBandwidth(long sbw);
void setLdoFlag();
void setCodingRate4(int denominator);
void setPreambleLength(long length);
void setSyncWord(int sw);
void enableCrc();
void disableCrc();
void enableInvertIQ();
void disableInvertIQ();
void setOCP(uint8_t mA);
void setGain(uint8_t gain);
uint8_t random();
void setPins(int ss, int reset, int dio0);
void setSPI(spi_inst_t& spi);
void setSPIFrequency(uint32_t frequency);
void dumpRegisters();
void explicitHeaderMode();
void implicitHeaderMode();
void handleDio0Rise();
uint8_t readRegister(uint8_t address);
void writeRegister(uint8_t address, uint8_t value);
uint8_t singleTransfer(uint8_t address, uint8_t value);
void onDio0Rise(uint gpio, uint32_t events);




#endif //PICO_CONTROLLER_LORA_H
