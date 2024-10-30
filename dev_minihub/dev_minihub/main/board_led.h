#pragma once
#ifndef _BOARD_LED_H_
#define _BOARD_LED_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "sdkconfig.h"
#include "driver/gpio.h"
#include "led_strip_spi.h"

#define LED_ALL    -1
#define LED_YELLOW 40
#define LED_GREEN  120
#define LED_MINT   130
#define LED_BLUE   240
#define LED_RED    360

/* LC8823 Luminous Intensity */
#define LED_RED_INTENSITY        230
#define LED_GREEN_INTENSITY      320
#define LED_BLUE_INTENSITY       80
#define LED_BRIGHTNESS_PERCENT   40

// #define LED_YELLOW_RATIO  0.25
static const float LED_BLUE_RATIO   = 1;
static const float LED_RED_RATIO    = (float)LED_BLUE_INTENSITY / (float)LED_RED_INTENSITY;
static const float LED_GREEN_RATIO  = (float)LED_BLUE_INTENSITY / (float)LED_GREEN_INTENSITY;

typedef struct {
    union {
        uint16_t h;
        uint16_t hue;
    };
    union {
        uint8_t s;
        uint8_t sat;
        uint8_t saturation;
    };
    union {
        uint8_t v;
        uint8_t val;
        uint8_t value;
    };
} hsv_state_t;

extern struct led_state_rgb {
    uint8_t element;
    uint8_t index;
    hsv_state_t current;
    hsv_state_t previous;
    char *name;
} led_rgb_state[LED_NUM];

void board_led_rgb_init(void);
void board_led_rgb_set(int8_t element, uint16_t color, uint8_t brightness);
void board_led_rgb_set_color(int8_t element, uint16_t color, bool set);
void board_led_rgb_set_saturation(int8_t element, uint8_t saturation);
void board_led_rgb_set_brightness(int8_t element, uint8_t brightness);
void board_led_rgb_flip_task(void *pvParameters);
void board_led_rgb_prov_none_task(void *pvParameters);
void board_led_rgb_prov_run_task(void *pvParameters);
void board_led_rgb_wifi_drop_task(void *pvParameters);
void board_led_rgb_hardware_fail_task(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif