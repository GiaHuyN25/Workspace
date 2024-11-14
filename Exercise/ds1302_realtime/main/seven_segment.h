#ifndef SEVEN_SEGMENT_H
#define SEVEN_SEGMENT_H
#include "esp_err.h"
#include "hal/gpio_types.h"

typedef enum{
    num_0 = 0b0111111, // 0
    num_1 = 0b0000110, // 1
    num_2 = 0b1011011, // 2
    num_3 = 0b1001111, // 3
    num_4 = 0b1100110, // 4
    num_5 = 0b1101101, // 5
    num_6 = 0b1111101, // 6
    num_7 = 0b0000111, // 7
    num_8 = 0b1111111, // 8
    num_9 = 0b1101111  // 9
} digit_to_segments_t;

int bit_convert(int n);
digit_to_segments_t get_bit(int i);
void number_display(int *ledPin, int number);

int power_of_10(int n);
void number_divide(int number, int *n);
void numberDisplay_4_Segment(int *Position, int number, int *ledPin);

#endif