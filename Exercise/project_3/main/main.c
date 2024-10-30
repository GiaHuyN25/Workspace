#include <stdio.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_rom_gpio.h>
#include "sdkconfig.h"
#include "input_iot.h"
#include "output_iot.h"

#define HI_GPIO     GPIO_NUM_1
#define BLINK_GPIO  GPIO_NUM_2
#define INPUT_GPIO  GPIO_NUM_4

void input_event_callback(int pin)
{
    if(pin == INPUT_GPIO){
        output_toggle(BLINK_GPIO);
    }
}

void app_main(void)
{
    output_init(BLINK_GPIO);
    output_set_level(BLINK_GPIO, ON);
    input_init(INPUT_GPIO, HI_TO_LO, PULL_UP);

    while(1){
        input_set_callback(input_event_callback);
    }
    
}
