#include <stdio.h>
#include <string.h>
#include <driver/gpio.h>
#include <driver/uart.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/queue.h>
#include <freertos/timers.h>
#include <gpio_init.h>

#define UART                UART_NUM_1
#define TX_PIN              GPIO_NUM_2
#define RX_PIN              GPIO_NUM_4
#define BUTTON              GPIO_NUM_12
#define BLINK               GPIO_NUM_26
#define POWER               GPIO_NUM_15

#define BIT_BLINK_EVENT     (1 << 0)
#define BIT_UART_EVENT      (1 << 1)

#define BUFFER              1024

static QueueHandle_t uart_queue;
static EventGroupHandle_t xEvent;
static TimerHandle_t xTimer[1];
static BaseType_t pxHigherPriorityTaskWoken;

static uint64_t ms = 500;

void UART_Task(void *pvParameter);
void Event_Task(void *pvParameter);

void button_callback(int pin);
void Timer_Callback(TimerHandle_t xTimer);
void blink(int pin);
uint64_t getNumber(char *str);

void app_main(void)
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .source_clk = UART_SCLK_APB,
    };
    uart_driver_install(UART, BUFFER * 2, BUFFER * 2, 20, &uart_queue, NULL);
    uart_param_config(UART, &uart_config);
    uart_set_pin(UART, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    input_init(BUTTON, HI_TO_LO, PULL_UP);
    input_output_init(BLINK, NO_INTR, NO_PULL, ON);
    output_init(POWER, NO_INTR, ON);

    xTaskCreate(UART_Task, "UART Task", 2*1024, NULL, 4, NULL);

    xTimer[0] = xTimerCreate("Timer Blink:", pdMS_TO_TICKS(ms), pdTRUE, (void *) 0, Timer_Callback);

    xEvent = xEventGroupCreate();

    input_set_callback(button_callback);

    xTaskCreate(Event_Task, "Event Task", 2* 1024, NULL, 4, NULL);
    
}

void button_callback(int pin)
{
    if (pin == BUTTON)
    {
        xEventGroupSetBitsFromISR(xEvent, BIT_BLINK_EVENT, &pxHigherPriorityTaskWoken);
    }
}

void Timer_Callback(TimerHandle_t xTimer)
{
    uint32_t ulCount;

    configASSERT( xTimer);

    ulCount = (uint32_t ) pvTimerGetTimerID(xTimer);

    switch(ulCount){
        case 0:
            blink(BLINK);
            break;
    }
}

void blink(int pin)
{
    int stt = gpio_get_level(BLINK);
    gpio_set_level(BLINK, 1 - stt);
    if(gpio_get_level(BUTTON) == ON)
    {
        xTimerStop(xTimer[0], portMAX_DELAY);
    }
}

uint64_t getNumber(char *str)
{
    uint64_t number;
    if(sscanf(str, "%*[^0-9]%lld", &number))
    {
        
        return number;
    }
    return 500;
}

void UART_Task(void *pvParameter)
{
    uart_event_t event;
    uint8_t *data = (uint8_t*) malloc(BUFFER);
    while(1)
    {
        if(xQueueReceive(uart_queue, (void *)&event, (TickType_t) portMAX_DELAY))
        {
            bzero(data, BUFFER);
            ESP_LOGI("UART:", "UART num %d event.", UART);
            switch(event.type)
            {
                case UART_DATA:
                    ESP_LOGI("UART:", "[UART DATA]: %d", event.size);
                    uart_read_bytes(UART, data, event.size, portMAX_DELAY);
                    uart_write_bytes(UART, (const char*) data, event.size);
                    ms = getNumber((char *)data);
                    ESP_LOGI("PERIOD:","%lld ms", ms);
                    xTimerChangePeriodFromISR(xTimer[0], pdMS_TO_TICKS(ms), pxHigherPriorityTaskWoken);
                    xEventGroupSetBits(xEvent, BIT_UART_EVENT);
                    break;
                case UART_FIFO_OVF:
                ESP_LOGI("UART:", "hw fifo overflow");
                    uart_flush_input(UART);
                    xQueueReset(uart_queue);
                    break;

                case UART_BUFFER_FULL:
                    uart_flush_input(UART);
                    xQueueReset(uart_queue);
                    break;

                case UART_BREAK:
                    ESP_LOGI("UART:", "uart rx break");
                    break;

                case UART_PARITY_ERR:
                    ESP_LOGI("UART:", "uart parity error");
                    break;

                case UART_FRAME_ERR:
                    ESP_LOGI("UART:", "uart frame error");
                    break;

                default:
                    ESP_LOGI("UART:", "uart event type: %d", event.type);
                break;
            }
        }
    }
}

void Event_Task(void *pvParameter)
{
    while(1)
    {
        EventBits_t xBit;
        xBit = xEventGroupWaitBits(xEvent,
                            BIT_BLINK_EVENT | BIT_UART_EVENT,
                            pdTRUE,
                            pdFALSE,
                            portMAX_DELAY);

        if(xBit & BIT_BLINK_EVENT)
        {
            ESP_LOGI("Blink Task:", "Start Blink.");
            xTimerStart(xTimer[0], 0);
        }
        if(xBit & BIT_UART_EVENT)
        {
            ESP_LOGI("UART:", "UART event completed.");
        }
    }
}