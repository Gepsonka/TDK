#include "lcd.h"

static char* TAG = "LCD";

extern SemaphoreHandle_t lg_state_mutex;
extern LandingGearState lg_state;


SemaphoreHandle_t lcd_mutex;

TaskHandle_t xLCDThrottleDisplayTaskHandler;


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

    lcd_mutex = xSemaphoreCreateMutex();
    if (lcd_mutex == NULL) {
        ESP_LOGE(TAG, "Could not create LCD mutex");
    }

    lcd_print_display_base();

    xTaskCreate(vLCDGeneralDataDisplay, "ThrottleDisplayTask", 2048, NULL, 1, &xLCDThrottleDisplayTaskHandler);
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
    lcd_set_cursor(FirstLine, 2);
    lcd_send_string("THR:");
    lcd_set_cursor(SecondLine, 0);
    lcd_send_string("Dir:");
    lcd_set_cursor(ThirdLine, 10);
    lcd_send_string("RTLG:");
    lcd_set_cursor(FourthLine, 0);
    lcd_send_string("Devices:");
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
    // ESP_LOGI("Thr percent", "%s", percentage_str_format);
    lcd_set_cursor(FirstLine, 4);
    lcd_send_string(percentage_str_format);
}

void lcd_print_joystick_data(){
    char x_data[] = "X:-100%";
    char y_data[] = "Y:-100%";
    char z_data[] = "Z:-100%";

    int8_t x_percentage = joystick_convert_current_joystick_x_direction_to_percentage();
    int8_t y_percentage = joystick_convert_current_joystick_y_direction_to_percentage();
    int8_t z_percentage = joystick_convert_current_joystick_rudder_direction_to_percentage();

    memset(x_data, ' ', sizeof(x_data));
    memset(y_data, ' ', sizeof(y_data));
    memset(z_data, ' ', sizeof(z_data));

    sprintf(x_data, "X:%d%%", x_percentage);
    sprintf(y_data, "Y:%d%%", y_percentage);
    sprintf(z_data, "Z:%d%%", z_percentage);

    if (x_percentage > -100 && x_percentage < -9) {
        memset(&x_data[6], ' ', 1);
    } else if (x_percentage > -10 && x_percentage < 0) {
        memset(&x_data[5], ' ', 2);
    } else if (x_percentage > -1 && x_percentage < 10) {
        memset(&x_data[4], ' ', 3);
    } else if (x_percentage > 9 && x_percentage < 100) {
        memset(&x_data[5], ' ', 2);
    } else if ( x_percentage == 100 ) {
        memset(&x_data[6], ' ', 1);
    }

    if (y_percentage > -100 && y_percentage < -9) {
        memset(&y_data[6], ' ', 1);
    } else if (y_percentage > -10 && y_percentage < 0) {
        memset(&y_data[5], ' ', 2);
    } else if (y_percentage > -1 && y_percentage < 10) {
        memset(&y_data[4], ' ', 3);
    } else if (y_percentage > 9 && y_percentage < 100) {
        memset(&y_data[5], ' ', 2);
    } else if ( y_percentage == 100 ) {
        memset(&y_data[6], ' ', 1);
    }

    if (z_percentage > -100 && z_percentage < -9) {
        memset(&z_data[6], ' ', 1);
    } else if (z_percentage > -10 && z_percentage < 0) {
        memset(&z_data[5], ' ', 2);
    } else if (z_percentage > -1 && z_percentage < 10) {
        memset(&z_data[4], ' ', 3);
    } else if (z_percentage > 9 && z_percentage < 100) {
        memset(&z_data[5], ' ', 2);
    } else if ( z_percentage == 100 ) {
        memset(&z_data[6], ' ', 1);
    }


    lcd_set_cursor(SecondLine, 4);
    lcd_send_string(x_data);
    lcd_set_cursor(SecondLine, 12);
    lcd_send_string(y_data);
    lcd_set_cursor(ThirdLine, 0);
    lcd_send_string(z_data);
}

void lcd_print_current_num_of_devices(uint8_t device_count) {
    char device_count_str[] = "Device:1000";
    memset(device_count_str, ' ', sizeof(device_count));

    sprintf(device_count_str, "Device:%d", device_count);
    lcd_set_cursor(FourthLine, 0);
    lcd_send_string(device_count_str);
}

void lcd_print_state_of_RTLG() {
    lcd_set_cursor(ThirdLine, 15);
    if (xSemaphoreTake(lg_state_mutex, portMAX_DELAY) == pdPASS) {
        switch (lg_state) {
            case RETRACTED:
                lcd_send_string("RETR");
            case EXTRACTED:
                lcd_send_string("EXTR");
        }
        xSemaphoreGive(lg_state_mutex);
    }
}


void vLCDGeneralDataDisplay(void* pvParameters) {
    while(1) {
        if (xSemaphoreTake(lcd_mutex, portMAX_DELAY) == pdPASS){
            lcd_print_current_throttle_percentage();
            lcd_print_joystick_data();
            lcd_print_state_of_RTLG();
            xSemaphoreGive(lcd_mutex);
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}