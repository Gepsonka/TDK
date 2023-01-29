#include "gps.h"
#include <nmea.h>


esp_err_t init_gps() {
    const char init_data[] = "$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29\r\n";

    return uart_write_bytes(GPS_UART_PORT_NUM, init_data, strlen(init_data));

}

esp_err_t gps_read_data(GPSData* gps_data){
    uint8_t buff[127];

    esp_err_t res = uart_read_bytes(GPS_UART_PORT_NUM, buff, 127, 1000 / portTICK_PERIOD_MS);
    printf("%s", buff);

    return res;

}