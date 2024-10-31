#include <stdio.h>
#include <stdint.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <freertos/event_groups.h>
#include "freertos/queue.h"
#include <esp_rom_gpio.h>
#include <esp_timer.h>
#include "driver/uart.h"
#include "sdkconfig.h"
#include "gpio_init.h"

#define UART        UART_NUM_0

#define UART_BIT    (1 << 0)
#define BLINK_BIT   (1 << 1)

#define BUF_SIZE    1024



static const char *TAG = "UART_Event";

static EventGroupHandle_t xEvent;
static QueueHandle_t uart_queue;
static uart_event_t event;

void uart_event_task(void *pvParameter);
void event_handler_task(void *pvParameter);
void uart_task();
void uart_event(QueueHandle_t uart_q);

void app_main(void)
{
    xEvent = xEventGroupCreate();

    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    uart_param_config(UART, &uart_config);
    uart_driver_install(UART, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart_queue, 0);

    uart_set_callback(uart_event_task);

    xTaskCreate(uart_event_task, "uart_event_task", BUF_SIZE * 2, NULL, 12, NULL);
    xTaskCreate(event_handler_task, "event_handler_task", 2048, NULL, 10, NULL);
}

void uart_event_task(void *pvParameter)
{
    uart_event(uart_queue);
}

void event_handler_task(void *pvParameter)
{
    EventBits_t uxBits;
    uxBits = xEventGroupWaitBits(xEvent, UART_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
    if(uxBits & UART_BIT)
    {
        ESP_LOGI(TAG,"UART event done.");
        uart_task();
    }
}

void uart_task()
{
    uint8_t *dtmp = (uint8_t) malloc(BUF_SIZE);
    if(event.type == UART_DATA)
    {
        int len = uart_read_bytes(UART, dtmp, event.size, portMAX_DELAY);
        if(len > 0)
        {
            ESP_LOGI(TAG, "Received: %s", (char *)dtmp);
        }
    }
}

void uart_event(QueueHandle_t uart_q)
{
    if(xQueueReceive(uart_q, (void*)&event, (TickType_t) portMAX_DELAY))
    {
        xEventGroupSetBits(xEvent, UART_BIT);
    }
}