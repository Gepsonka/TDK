#include "driver/uart.h"
#include <string.h>

#ifndef GPS_H
#define GPS_H

#define GPS_UART_PORT_NUM UART_NUM_2

typedef struct {
    uint8_t time;
    uint8_t status;
    uint8_t latitude;
    uint8_t latitude_hemisphere;
    uint8_t longitude;
    uint8_t longitude_hemisphere;
    uint8_t speed;
    uint8_t track_angle;
    uint8_t date;
    uint8_t magnetic_variation;
    uint8_t magnetic_variation_direction;
    uint8_t mode;
} GPSData;

esp_err_t init_gps();
esp_err_t gps_read_data(GPSData* gps_data);




#endif
