#ifndef GPIO_INIT_H
#define GPIO_INIT_H
#include "esp_err.h"
#include "hal/gpio_types.h"

typedef void (*input_callback_t) (int);
typedef void (*uart_callback_t) (int);

typedef enum {
    NO_INTR = 0,
    LO_TO_HI = 1,
    HI_TO_LO = 2,
    ANY_EDLE = 3,
}   interrupt_type_edle_t;

typedef enum{
    OFF,
    ON
} state_t;

typedef enum {
    NO_PULL,
    PULL_DOWN,
    PULL_UP,
} pull_state_t;

esp_err_t input_init(gpio_num_t gpio_num, interrupt_type_edle_t type, pull_state_t state);
esp_err_t output_init(gpio_num_t gpio_num, interrupt_type_edle_t type_intr, state_t state);
esp_err_t input_output_init(gpio_num_t gpio_num, interrupt_type_edle_t type_intr, pull_state_t state, state_t level);

esp_err_t input_io_get_level(gpio_num_t gpio_num);

void input_set_callback(void * cb);
void uart_set_callback(void *cb);

#endif