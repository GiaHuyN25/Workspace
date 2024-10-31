#include <stdio.h>
#include <stdint.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <freertos/event_groups.h>
#include <esp_rom_gpio.h>
#include <esp_timer.h>
#include "sdkconfig.h"
#include "gpio_init.h"
#include "timer_button.h"

static const char *TAG = "Exercise_2";

#define BUTTON                      GPIO_NUM_5
#define POWER                       GPIO_NUM_16

#define BUFFER                      1024

#define BIT_EVENT_PRESS_BUTTON	    ( 1 << 0 )

static EventGroupHandle_t xEvent;

static uint64_t duration = 0;

void timer_duration_handle(uint64_t time);

void vTask1(void *pvParameter);

void input_timer(int pin);

void app_main(void)
{
    output_init(POWER, NO_INTR, ON);
    input_init(BUTTON, ANY_EDLE, PULL_UP);

    input_set_callback(input_timer);

    xEvent = xEventGroupCreate();

    xTaskCreate(vTask1, "vTask_Button:", 2*1024, NULL, 4, NULL);
}

void input_timer(int pin)
{
    if(pin == BUTTON)
    {
        BaseType_t pxHigherPriorityTaskWoken;
        xEventGroupSetBitsFromISR(xEvent, BIT_EVENT_PRESS_BUTTON, &pxHigherPriorityTaskWoken);
    }
}

void vTask1(void *pvParameter)
{
    while(1){
        EventBits_t uxBits;
        uxBits = xEventGroupWaitBits(
               xEvent,
               BIT_EVENT_PRESS_BUTTON,
               pdTRUE,
               pdFALSE,
               portMAX_DELAY);

        if (uxBits & BIT_EVENT_PRESS_BUTTON)
        {
            duration = hold_time_count_ms(BUTTON);
            if (gpio_get_level(BUTTON) == ON)
            {
                ESP_LOGI(TAG,"Hold duration: %lld ms", duration);
                timer_duration_handle(duration);
            }
        }
    }
}

void timer_duration_handle(uint64_t time)
{
    int class = duration_classification(time);
    switch(class)
    {
        case SHORT_TIME:
            ESP_LOGI(TAG,"Short time duration.");
            break;
        case NORMAL_TIME:
            ESP_LOGI(TAG,"Normal time duration.");
            break;
        case LONG_TIME:
            ESP_LOGI(TAG,"Long time duration.");
            break;
    }

}