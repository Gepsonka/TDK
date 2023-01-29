#include "driver/uart.h"
#include "uart.h"

void init_uart() {
  uart_config_t uart_config = {
        .baud_rate = 9600  ,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
  };

  uart_param_config(UART_NUM_2, &uart_config);
  uart_driver_install(UART_NUM_2, UART_RX_BUFF_SIZE, 0, 0, NULL, 0);
  uart_set_pin(UART_NUM_2, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}



