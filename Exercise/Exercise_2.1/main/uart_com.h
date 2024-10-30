#ifndef UART_COM_H
#define UART_COM_H
#include "esp_err.h"
#include "hal/gpio_types.h"

#define DEFAULT_BUFFER      1024

typedef void (*uart_callback_t) (int);
esp_err_t uart_init(QueueHandle_t *uart_queue);
void uart_set_callback(void * cb);

#endif