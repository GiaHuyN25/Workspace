#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"

static const int RX_SIZE = 1024;

#define TX_PIN GPIO_NUM_17
#define RX_PIN GPIO_NUM_16
#define output GPIO_MODE_OUTPUT
#define input GPIO_MODE_INPUT
#define RX_BUF_SIZE 1024


void init_uart(void);
void init_gpio(int gpio, int mode);

void tx_task(void *arg);
void rx_task(void *arg);

void read_gpio(void *arg);

void app_main(void)
{
    
}

void init_uart(void){
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

void init_gpio(int gpio, int mode){
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;   // Không sử dụng ngắt
    io_conf.mode = mode;         // Chế độ đầu ra
    io_conf.pin_bit_mask = (1ULL << gpio); // Chọn chân LED
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
}