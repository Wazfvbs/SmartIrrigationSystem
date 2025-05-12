#ifndef UART_RECEIVER_H
#define UART_RECEIVER_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"

#define UART_PORT_NUM      UART_NUM_1
#define UART_BAUD_RATE     115200
#define UART_RX_BUF_SIZE   1024
#define UART_TX_BUF_SIZE   512

void uart_receiver_init(void);

#endif // UART_RECEIVER_H
