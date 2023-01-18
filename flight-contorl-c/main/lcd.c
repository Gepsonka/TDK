#include <string.h>


#define LCD_DRIVER_I2C_ADDR 0x27


static void send_cmd(i2c_port_t i2c_num, uint8_t cmd) {
    uint8_t data_u;
    uint8_t data_l;

    uint8_t data[4];

    data_u = cmd & 0xf0;
    data_l = (cmd << 4) & 0xf0;

    data[0] = data_u|0x0C;  //en=1, rs=0
    data[1] = data_u|0x08;  //en=0, rs=0
    data[2] = data_l|0x0C;  //en=1, rs=0
    data[3] = data_l|0x08;  //en=0, rs=0

    esp_err_t ret = i2c_master_write_slave(0, LCD_DRIVER_I2C_ADDR, data, 4);

    if (ret != ESP_OK) {
        printf("LCD cmd failed");
    }


}

static void send_data(i2c_port_t i2c_num, uint8_t data_unit) {
    uint8_t data_u;
    uint8_t data_l;

    uint8_t data[4];

    data_u = data_unit & 0xf0;
    data_l = (data_unit << 4) & 0xf0;

    data[0] = data_u|0x0D;  //en=1, rs=1
    data[1] = data_u|0x09;  //en=0, rs=1
    data[2] = data_l|0x0D;  //en=1, rs=1
    data[3] = data_l|0x09;  //en=0, rs=1

    esp_err_t ret = i2c_master_write_slave(0, LCD_DRIVER_I2C_ADDR, data, 4);

    if (ret != ESP_OK) {
        printf("LCD cmd failed");
    }


}



void init_lcd(i2c_port_t i2c_num) {
    vTaskDelay(50 / portTICK_PERIOD_MS);
    send_cmd(0, 0x30);
    vTaskDelay(5 / portTICK_PERIOD_MS);
    send_cmd(0, 0x30);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    send_cmd(0, 0x30);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    send_cmd(0, 0x20);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    vTaskDelay(50 / portTICK_PERIOD_MS);
    send_cmd(0, 0x28);
    vTaskDelay(5 / portTICK_PERIOD_MS);
    send_cmd(0, 0x08);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    send_cmd(0, 0x01);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    send_cmd(0, 0x06);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    send_cmd(0, 0x0c);
    vTaskDelay(10 / portTICK_PERIOD_MS);
}


void lcd_send_string(i2c_port_t i2c_num, char* string) {
    for (int i=0; i < strlen(string);i++) {
        send_data(0, string[i]);
    }
}


