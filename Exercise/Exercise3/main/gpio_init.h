#ifndef GPIO_INIT_H
#define GPIO_INIT_H
#include "esp_err.h"
#include "hal/gpio_types.h"

//Callback pointer for input GPIO
typedef void (*input_callback_t) (int);

//Interrupt type for GPIO 
typedef enum {
    NO_INTR = 0,
    LO_TO_HI = 1,
    HI_TO_LO = 2,
    ANY_EDLE = 3,
}   interrupt_type_edle_t;

//GPIO state 
typedef enum{
    OFF,
    ON
} state_t;

//Pull resistor for GPIO
typedef enum {
    NO_PULL,
    PULL_DOWN,
    PULL_UP,
} pull_state_t;

//Initiating for input GPIO
esp_err_t input_init(gpio_num_t gpio_num, interrupt_type_edle_t type, pull_state_t state);

//Initiating for output GPIO
esp_err_t output_init(gpio_num_t gpio_num, interrupt_type_edle_t type_intr, state_t state);

//Initiating for input_output GPIO
esp_err_t input_output_init(gpio_num_t gpio_num, interrupt_type_edle_t type_intr,
                                pull_state_t state, state_t level);

//Initiating for adc GPIO
void adc_gpio_init(adc_channel_t adc_channel, adc_unit_t adc_unit, 
                            adc_oneshot_unit_handle_t *adc_handle);


esp_err_t input_io_get_level(gpio_num_t gpio_num);

//Set callback pointer for input GPIO
void input_set_callback(void * cb);

#endif