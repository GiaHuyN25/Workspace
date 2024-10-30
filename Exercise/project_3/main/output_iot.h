#ifndef OUTPUT_IO_H
#define OUTPUT_IO_H
#include "esp_err.h"
#include "hal/gpio_types.h"

void output_init(gpio_num_t gpio_num);
int  output_get_level(gpio_num_t gpio_num);
void output_set_level(gpio_num_t gpio_num, state_t level);
void output_toggle(gpio_num_t gpio_num);

#endif