#include "spi.h"

esp_err_t init_spi() {
    spi_bus_config_t buscfg={
        .miso_io_num = SPI_MISO_PIN,
        .mosi_io_num = SPI_MOSI_PIN,
        .sclk_io_num = SPI_SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32,
    };
    //Initialize the SPI bus
    esp_err_t ret = spi_bus_initialize(SPI_HOST_NUM, &buscfg, SPI_DMA_CH_AUTO);
    return ret;
}