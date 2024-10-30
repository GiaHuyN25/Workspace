#include <stdio.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_rom_gpio.h>
#include "sdkconfig.h"
#include "input_iot.h"

#define HI_GPIO     GPIO_NUM_1
#define BLINK_GPIO  GPIO_NUM_2
#define INPUT_GPIO  GPIO_NUM_4

#define ON      1
#define OFF     0

static const char *TAG = "Project_2";

void input_event_callback(int pin)
{
    if(pin == INPUT_GPIO){
        static int stt = 0;
        gpio_set_level(BLINK_GPIO, stt);
        stt = 1- stt;
        printf("Callback triggered.\n");
    }
}

void app_main(void)
{
    esp_rom_gpio_pad_select_gpio(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(BLINK_GPIO,ON);

    input_init(INPUT_GPIO, HI_TO_LO, PULL_UP);

    while(1){
        input_set_callback(input_event_callback);
    }
    
}
