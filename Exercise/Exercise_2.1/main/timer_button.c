#include <stdio.h>
#include <stdint.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_attr.h"
#include "gpio_init.h"
#include "timer_button.h"

static uint64_t start_timer = 0;
static uint64_t timer_duration = 0;

uint64_t hold_time_count_ms(gpio_num_t gpio_num)
{
    //Need to use ANY_EDGE INTR_TYPE for input config
    if(gpio_get_level(gpio_num) == OFF){
        start_timer = esp_timer_get_time();
    }else if(gpio_get_level(gpio_num) == ON){
        if(start_timer != 0){
            timer_duration = esp_timer_get_time() - start_timer;
            start_timer = 0;
        }
    }

    return timer_duration/1000;
}

int duration_classification(uint64_t time)
{
    //In millisecond (ms)
    if (time<1000)
    {
        return SHORT_TIME;
    }
    else if(time>=1000 && time <= 3000)
    {
        return NORMAL_TIME;
    }
    return LONG_TIME;
}