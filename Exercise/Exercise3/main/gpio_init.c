#include <stdio.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include <esp_adc/adc_oneshot.h>
#include "esp_attr.h"
#include "gpio_init.h"


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

esp_err_t output_init(gpio_num_t gpio_num, interrupt_type_edle_t type_intr, state_t state)
{
    gpio_config_t io_conf = {
        .intr_type = type_intr,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << gpio_num),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };

    gpio_config(&io_conf);
    gpio_set_level(gpio_num, state);

    return ESP_OK;
}

esp_err_t input_output_init(gpio_num_t gpio_num, interrupt_type_edle_t type_intr, pull_state_t state, state_t level)
{
    gpio_config_t io_conf = {
        .intr_type = type_intr,
        .mode = GPIO_MODE_INPUT_OUTPUT,
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

    gpio_set_level(gpio_num, level);

    return ESP_OK;
}

void adc_gpio_init(adc_channel_t adc_channel, adc_unit_t adc_unit, 
                            adc_oneshot_unit_handle_t *adc_handle)
{
    adc_oneshot_unit_init_cfg_t adc_init_config = {
        .unit_id = adc_unit,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_init_config, adc_handle) != ESP_OK);

    adc_oneshot_chan_cfg_t adc_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(*adc_handle, adc_channel, &adc_config) != ESP_OK);
    printf("Initiated ADC.\n");
}