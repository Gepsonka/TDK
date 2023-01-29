#include "driver/i2c.h"

#ifndef I2C_H
#define I2C_H

// Project specific I2C setup
#define I2C_MASTER_SCL_IO    22    /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO    21    /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM 0   /*!< I2C port number for master dev */
#define I2C_MASTER_TX_BUF_DISABLE 0
#define I2C_MASTER_RX_BUF_DISABLE 0   /*!< I2C master do not need buffer */
#define I2C_MASTER_FREQ_HZ    100000     /*!< I2C master clock frequency */
#define ACK_CHECK_EN 1
#define NACK_VAL 0



void i2c_master_init();
esp_err_t i2c_master_write_slave(i2c_port_t i2c_num, uint8_t slave_addr, uint8_t *data_wr, size_t size);
esp_err_t i2c_master_read_slave(i2c_port_t i2c_num, uint8_t slave_addr, uint8_t *data_rd, size_t size);
esp_err_t i2c_read_register(i2c_port_t i2c_num, uint8_t slave_address, uint8_t reg_address, uint8_t *data, size_t data_len);

#endif