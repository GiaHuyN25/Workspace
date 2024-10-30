#include <stdio.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include "esp_attr.h"
#include "input_iot.h"

input_callback_t input_callback = NULL;

static void IRAM_ATTR gpio_input_handler(void *arg)
{
    int gpio_num = (uint32_t) arg;
    input_callback(gpio_num);
}

esp_err_t input_init(gpio_num_t gpio_num, interrupt_type_edle_t type_intr, pull_state_t state)
{
    gpio_config_t io_conf = {
        .intr_type = type_intr,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << gpio_num),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };

    switch(state)
    {
        case NO_PULL:
            break;
        case PULL_DOWN:
            io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
            break;
        case PULL_UP:
            io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
            break;
        default:
            break;
    }
    
    gpio_config(&io_conf);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(gpio_num, gpio_input_handler, (void *) gpio_num);

    return ESP_OK;
}

esp_err_t input_io_get_level(gpio_num_t gpio_num)
{
    return gpio_get_level(gpio_num);
}

void input_set_callback(void * cb)
{
    input_callback = cb;
}

