#include "driver/uart.h"
#include "uart.h"

void init_uart() {
  uart_config_t uart_config = {
        .baud_rate = 9600  ,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOW
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
  };

  uart_param_config(UART_NUM_2, &uart_config);
  uart_driver_install(UART_NUM_2, 4096, 0, 0, NULL, 0);
}



