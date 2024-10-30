#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "sdkconfig.h"

#define RED_LED     GPIO_NUM_5

#define ON 1
#define OFF 0

#define BUFFER 1024

static const char *TAG = "LED_UART";
static QueueHandle_t uart0_queue;

void gpio_init(int GPIO_NUM);

void uart_task(void *pvParamater, int GPIO_NUM);

void app_main(void)
{
    gpio_init(RED_LED);

    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    uart_driver_install(UART_NUM_0, BUFFER*2, BUFFER*2, 20, &uart0_queue, 0);
    uart_param_config(UART_NUM_0, &uart_config);

    uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    xTaskCreate(uart_task, "LED_UART", 2048, NULL, 12, NULL);
}

void gpio_init(int GPIO_NUM){
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;   
    io_conf.mode = GPIO_MODE_OUTPUT;         
    io_conf.pin_bit_mask = (1ULL << GPIO_NUM); 
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
}

void uart_task(void* pvParameter, int GPIO_NUM){
    uart_event_t event;
    size_t buffered_size;
    uint8_t* dtmp = (uint8_t*) malloc(BUFFER);
    for (;;) {
        //Waiting for UART event.
        if (xQueueReceive(uart0_queue, (void *)&event, (TickType_t)portMAX_DELAY)) {
            bzero(dtmp, BUFFER);
            ESP_LOGI(TAG, "uart[%d] event:", UART_NUM_0);
            ESP_LOGI(TAG, "[UART DATA]: %d", event.size);
            uart_read_bytes(UART_NUM_0, dtmp, event.size, portMAX_DELAY);
            ESP_LOGI(TAG, "[DATA EVT]:");
            uart_write_bytes(UART_NUM_0, (const char*) dtmp, event.size);
        }

        char on_state[4] = "on";
        char off_state[4] = "off";

        dtmp[strcspn((char *) dtmp, "\n")] = '\0';

        if(strcmp(on_state, (char *) dtmp) == 0){
            gpio_set_level(RED_LED, ON);
            ESP_LOGI(TAG, "STATUS ON.");
        }else if(strcmp(off_state, (char *) dtmp) == 0){
            gpio_set_level(RED_LED, OFF);
            ESP_LOGI(TAG,"STATUS OFF.");
        }else{
            ESP_LOGI(TAG,"UNKNOWN STATUS.");
        }
    }
    free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL);
}