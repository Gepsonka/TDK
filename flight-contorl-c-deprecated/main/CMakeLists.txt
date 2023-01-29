set(COMPONENT_SRCS "main.c" "i2c.c"  "gps.c" "uart.c" "lcd.c" "spi.c")
set(COMPONENT_ADD_INCLUDEDIRS "")


idf_component_register()