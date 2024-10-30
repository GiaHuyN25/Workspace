#include <stdio.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <freertos/event_groups.h>
#include <esp_rom_gpio.h>
#include "sdkconfig.h"
#include "input_iot.h"

#define BUTTON              GPIO_NUM_21
#define BLINK_GPIO          GPIO_NUM_2
#define CHECK_STATUS_PIN    GPIO_NUM_4

#define BIT_EVENT_PRESS_BUTTON	( 1 << 0 )
#define BIT_EVENT_UART_RECV     ( 1 << 1 )

static const char *TAG = "freeRTOS_study";

TimerHandle_t xTimers[2];

EventGroupHandle_t xEventGroup;

void output_config(gpio_num_t gpio_num);

void vTask1( void * pvParameters );

void vTimerCallback( TimerHandle_t xTimer );

void button_callback(int pin);

void app_main(void)
{
    xTimers[0] = xTimerCreate("Timer Blink:", pdMS_TO_TICKS(500), pdTRUE, (void *) 0, vTimerCallback);
    xTimers[1] = xTimerCreate("Timer Print:", pdMS_TO_TICKS(1000), pdTRUE, (void *) 1, vTimerCallback);

    
    input_init(BUTTON, HI_TO_LO, PULL_UP);
    input_init(CHECK_STATUS_PIN, NO_INTR, NO_PULL);
    output_config(BLINK_GPIO);
    gpio_set_level(BLINK_GPIO, ON);

    input_set_callback(button_callback);

    xEventGroup = xEventGroupCreate();
    // xTimerStart(xTimers[0], 0);
    // xTimerStart(xTimers[1], 0);

    xTaskCreate(vTask1, "vTask_1", 2*1024, NULL, 4, NULL);

}


void output_config(gpio_num_t gpio_num)
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << gpio_num),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    gpio_config(&io_conf);
}

void output_toggle(gpio_num_t gpio_num)
{
    int stt = input_io_get_level(CHECK_STATUS_PIN);
    gpio_set_level(gpio_num, 1 - stt);
    vTaskDelay(1000/portTICK_PERIOD_MS);
}

void button_callback(int pin)
{
    if (pin == BUTTON)
    {
        BaseType_t pxHigherPriorityTaskWoken;
        xEventGroupSetBitsFromISR(xEventGroup, BIT_EVENT_PRESS_BUTTON, &pxHigherPriorityTaskWoken);
    }
}

void vTask1( void * pvParameters )
{
    while(1){
        EventBits_t uxBits;
        uxBits = xEventGroupWaitBits(
               xEventGroup,                                     /* The event group being tested. */
               BIT_EVENT_PRESS_BUTTON | BIT_EVENT_UART_RECV,    /* The bits within the event group to wait for. */
               pdTRUE,                                          /* BIT_0 & BIT_4 should be cleared before returning. */
               pdFALSE,                                         /* Don't wait for both bits, either bit will do. */
               portMAX_DELAY);

        if (uxBits & BIT_EVENT_PRESS_BUTTON)
        {
            ESP_LOGI(TAG,"Blink.");
            output_toggle(BLINK_GPIO);
        }
        if (uxBits & BIT_EVENT_UART_RECV)
        {
            printf("UART Received.\n");
            // ...
        }
    }
    
}

void vTimerCallback( TimerHandle_t xTimer )
{
    uint32_t ulCount;

    //If pxTimer == NULL => Do smt
    configASSERT( xTimer );

    //ulCount = TimerID
    ulCount = ( uint32_t ) pvTimerGetTimerID( xTimer );

    switch(ulCount)
    {
        case 0:
            //Blink Timer
            ESP_LOGI(TAG, "Blink.");
            output_toggle(BLINK_GPIO);
            break;
        case 1:
            //Print Timer
            printf("Hello\n");
            break;
        default:
            break;
    }
 }