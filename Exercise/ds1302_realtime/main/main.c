#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <esp_timer.h>
#include <esp_log.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <gpio_init.h>
#include <seven_segment.h>
#include <realtime.h>

void app_main(void)
{
    realtime_t currentTime = {
        .year = 2024,
        .month = 14,
        .day = 90,
        .hour = 60,
        .min = 90,
        .sec = 100,
    };
    while (1){
        time_modify(&currentTime);
        time_print(currentTime);
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}