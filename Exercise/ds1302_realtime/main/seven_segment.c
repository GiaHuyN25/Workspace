#include <stdio.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <gpio_init.h>
#include "seven_segment.h"

int bit_convert(int n) 
{
    //convert to bit with n is position of bit 1
    //only available for 1 bit_1 in the array of bit, for example 00010000 with n = 4
    int bit = 1;
    for (int i = 0; i<n; i++)
    {
        bit *= 2;
    }
    return bit;
}

digit_to_segments_t get_bit(int i)
{
    //get the bit array that represents the number of 7-segment display
    switch(i){
        case 0: return num_0;
        case 1: return num_1;
        case 2: return num_2;
        case 3: return num_3;
        case 4: return num_4;
        case 5: return num_5;
        case 6: return num_6;
        case 7: return num_7;
        case 8: return num_8;
        case 9: return num_9;
        default: 
        ESP_LOGI("SEVEN-SEGMENT", "Out of range (%d), return 0.", i);
        return num_0;
    }
}

void number_display(int *ledPin, int number)
{
    //display wanted number
    for (int i = 0; i<7; i++)
    {
        gpio_set_level(*(ledPin+i), (get_bit(number) & bit_convert(i)) ? 1: 0);
    }
}

int power_of_10(int n)
{
    int result = 1;
    for (int i = 0; i < n; i++)
    {
        result *= 10; 
    }
    return result;
}


void number_divide(int number, int *n)
{
    int temp = number;
    for (int i = 0; i < 4; i++)
    {
        int divisor = power_of_10(3 - i);
        *(n + i) = temp / divisor;
        temp = temp % divisor;
    }
}


void numberDisplay_4_Segment(int *Position, int number, int *ledPin)
{
    int a[4];
    number_divide(number, a);
    for (int i = 0; i<4; i++)
    {
        gpio_set_level(*(Position+i), ON);
        number_display(ledPin, a[i]);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(*(Position+i), OFF);
    }
}