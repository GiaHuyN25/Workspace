#include <driver/gpio.h>

void gpio_init(gpio_num_t gpio_num)
{
    gpio_config_t io_conf = {
        .intr_type = 0,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << gpio_num),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    gpio_config(&io_conf);
}

void app_main(void)
{
    gpio_init(GPIO_NUM_2);
    gpio_set_level(GPIO_NUM_2, 1);
    gpio_init(GPIO_NUM_4);
    gpio_set_level(GPIO_NUM_4, 1);
}