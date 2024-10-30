#ifndef INPUT_IO_H
#define INPUT_IO_H
#include "esp_err.h"
#include "hal/gpio_types.h"

typedef void (*input_callback_t) (int);

typedef enum{
    OFF,
    ON
} state_t;

typedef enum {
    LO_TO_HI = 1,
    HI_TO_LO = 2,
    ANY_EDLE = 3,
}   interrupt_type_edle_t;

typedef enum {
    NO_PULL,
    PULL_DOWN,
    PULL_UP,
} pull_state_t;

esp_err_t input_init(gpio_num_t gpio_num, interrupt_type_edle_t type, pull_state_t state);
esp_err_t input_io_get_level(gpio_num_t gpio_num);
void input_set_callback(void * cb);

#endif