#include "lcd.h"



extern Joystick_Direction joysctick_state;
extern SemaphoreHandle_t joystick_semaphore_handle;

static void send_cmd(uint8_t cmd) {
    uint8_t data_u;
    uint8_t data_l;

    uint8_t data[4];

    data_u = cmd & 0xf0;
    data_l = (cmd << 4) & 0xf0;

    data[0] = data_u|0x0C;  //en=1, rs=0
    data[1] = data_u|0x08;  //en=0, rs=0
    data[2] = data_l|0x0C;  //en=1, rs=0
    data[3] = data_l|0x08;  //en=0, rs=0

    esp_err_t ret = i2c_master_write_slave(LCD_I2C_DEFAULT_PORT, LCD_DRIVER_I2C_ADDR, data, 4);

    if (ret != ESP_OK) {
        printf("LCD cmd failed");
    }


}

static void send_data( uint8_t data_unit) {
    uint8_t data_u;
    uint8_t data_l;

    uint8_t data[4];

    data_u = data_unit & 0xf0;
    data_l = (data_unit << 4) & 0xf0;

    data[0] = data_u|0x0D;  //en=1, rs=1
    data[1] = data_u|0x09;  //en=0, rs=1
    data[2] = data_l|0x0D;  //en=1, rs=1
    data[3] = data_l|0x09;  //en=0, rs=1

    esp_err_t ret = i2c_master_write_slave(LCD_I2C_DEFAULT_PORT, LCD_DRIVER_I2C_ADDR, data, 4);

    if (ret != ESP_OK) {
        printf("LCD cmd failed");
    }


}


void init_lcd() {
    vTaskDelay(50 / portTICK_PERIOD_MS);
    send_cmd(0x30);
    vTaskDelay(5 / portTICK_PERIOD_MS);
    send_cmd(0x30);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    send_cmd(0x30);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    send_cmd(0x20);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    vTaskDelay(50 / portTICK_PERIOD_MS);
    send_cmd(0x28);
    vTaskDelay(5 / portTICK_PERIOD_MS);
    send_cmd(0x08);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    send_cmd(0x01);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    send_cmd(0x06);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    send_cmd(0x0c);
    vTaskDelay(10 / portTICK_PERIOD_MS);
}

void lcd_clear_screen() {
    send_cmd(0x01);
    vTaskDelay(1 / portTICK_PERIOD_MS);
}

void lcd_send_string(char* string) {
    for (int i=0; i < strlen(string);i++) {
        send_data(string[i]);
    }
}

void lcd_set_cursor(LCD_LineNumber line_num, uint8_t index) {
    switch (line_num) {
        case FirstLine:
            send_cmd(0x80|(0x00+index));
            break;
        case SecondLine:
            send_cmd(0x80|(0x40+index));
            break;
        case ThirdLine:
            send_cmd(0x80|(0x14+index));
            break;
        case FourthLine:
            send_cmd(0x80|(0x54+index));
            break;
    }
}

// To be modified later! Adding LoRa status and gps status
void lcd_print_display_base() {
    lcd_clear_screen();
    lcd_set_cursor(FirstLine, 0);
    lcd_send_string("THR:");
    lcd_set_cursor(SecondLine, 0);
    lcd_send_string("Dir:");
}

void lcd_print_current_throttle_percentage() {
    uint16_t raw_val;
    adc2_get_raw(ADC_CHANNEL, ADC_WIDTH, &raw_val);
    uint8_t percentage_val = throttle_convert_to_percentage(raw_val);
    char percentage_str_format[5] = "    ";
    sprintf(percentage_str_format, "%d%%", percentage_val);
    if (percentage_val < 10) {
        memset(&percentage_str_format[2], ' ', 2);
    } else if (percentage_val < 100) {
        memset(&percentage_str_format[3], ' ', 1);
    }
    ESP_LOGI("Thr percent", "%s", percentage_str_format);
    lcd_set_cursor(FirstLine, 4);
    lcd_send_string(percentage_str_format);
}

void lcd_print_throttle_percentage(uint16_t raw_val){
    fflush(stdout);
    uint8_t percentage_val = throttle_convert_to_percentage(raw_val);
    char percentage_str_format[5] = "    ";
    snprintf(percentage_str_format, 5, "%d%%", percentage_val);
    // percentage_str_format[3] = '%';
    printf(percentage_str_format, "\n");
    lcd_set_cursor(FirstLine, 3);
    lcd_send_string(percentage_str_format);
}

void lcd_print_joystick_direction(){
    lcd_set_cursor(SecondLine, 4);
    if (xSemaphoreTake(joystick_semaphore_handle, portMAX_DELAY) == pdTRUE) {
        switch (joysctick_state) {
            case NA:
                lcd_send_string("NA");
                break;
            case NORTH:
                lcd_send_string("N ");
                break;
            case SOUTH:
                lcd_send_string("S ");
                break;
            case NORTH_EAST:
                lcd_send_string("NE");
                break;
            case NORTH_WEST:
                lcd_send_string("NW");
                break;
            case SOUTH_EAST:
                lcd_send_string("SE");
                break;
            case SOUTH_WEST:
                lcd_send_string("SW");
                break;
            case EAST:
                lcd_send_string("E ");
                break;
            case WEST:
                lcd_send_string("W ");
                break;
        }
        xSemaphoreGive(joystick_semaphore_handle);
    }
    
}

void lcd_print_current_joystick_direction(uint8_t joystick_direction){
    lcd_set_cursor(SecondLine, 4);
    switch (joystick_direction) {
        case NA:
            lcd_send_string("NA");
            break;
        case NORTH:
            lcd_send_string("N ");
            break;
        case SOUTH:
            lcd_send_string("S ");
            break;
        case NORTH_EAST:
            lcd_send_string("NE");
            break;
        case NORTH_WEST:
            lcd_send_string("NW");
            break;
        case SOUTH_EAST:
            lcd_send_string("SE");
            break;
        case SOUTH_WEST:
            lcd_send_string("SW");
            break;
        case EAST:
            lcd_send_string("E ");
            break;
        case WEST:
            lcd_send_string("W ");
            break;
    }
}
