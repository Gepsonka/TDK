#include "lcd.c"
#include "driver/i2c.h"

enum LineNumber {
    FirstLine = 0x80|0x00,
    SecondLine = 0x80|0x40,
    ThirdLine = 0x80|0x14,
    FourthLine = 0x80|0x54
}




struct LCD {
    
}