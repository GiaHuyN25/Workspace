#include <stdio.h>
#include <string.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include "driver/uart.h"
#include "esp_attr.h"
#include "esp_intr_alloc.h"
#include "gpio_init.h"
#include "uart_com.h"

uart_callback_t uart_callback = NULL;

esp_err_t uart_init(QueueHandle_t *uart_queue)
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_driver_install(UART_NUM_1, DEFAULT_BUFFER * 2, DEFAULT_BUFFER * 2, 20, uart_queue, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, GPIO_NUM_13, GPIO_NUM_14, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    
    return ESP_OK;
}

static void IRAM_ATTR uart_input_handler(void *arg)
{
    int uart = (uint32_t) arg;
    uart_callback(uart);
}

void uart_set_callback(void * cb)
{
    uart_callback = cb;
}