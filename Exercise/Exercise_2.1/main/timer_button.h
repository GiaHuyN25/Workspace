#ifndef TIMER_BUTTON_H
#define TIMER_BUTTON_H
#include "esp_err.h"
#include "hal/gpio_types.h"

typedef enum{
    SHORT_TIME = 1,
    NORMAL_TIME = 2,
    LONG_TIME = 3
}timer_duration_t;

int duration_classification(uint64_t time);

uint64_t hold_time_count_ms(gpio_num_t gpio_num);

#endif