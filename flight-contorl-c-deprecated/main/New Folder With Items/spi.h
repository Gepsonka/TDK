#include "driver/spi_master.h"


#ifndef SPI_H
#define SPI_H

#define SPI_MISO_PIN 23
#define SPI_MOSI_PIN 19
#define SPI_SCLK_PIN 18

#define SPI_HOST_NUM HSPI_HOST

esp_err_t init_spi();




#endif