#include <stdio.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include "esp_attr.h"
#include "input_iot.h"
#include "output_iot.h"

void output_init(gpio_num_t gpio_num)
{
    gpio_config_t io_conf = {
        .intr_type = 0,
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pin_bit_mask = (1ULL << gpio_num),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    
    gpio_config(&io_conf);
}

int output_get_level(gpio_num_t gpio_num)
{
    return gpio_get_level(gpio_num);
}

void output_set_level(gpio_num_t gpio_num, state_t level)
{
    gpio_set_level(gpio_num, level);
}

void output_toggle(gpio_num_t gpio_num)
{
    int old_level = output_get_level(gpio_num);
    gpio_set_level(gpio_num, 1 - old_level);
}