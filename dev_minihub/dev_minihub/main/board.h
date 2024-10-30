// Copyright 2017-2019 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once
#ifndef _BOARD_H_
#define _BOARD_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "sdkconfig.h"
#include "driver/gpio.h"
#include "soc/soc_caps.h"
#include "driver/gpio_filter.h"
#include "esp_ble_mesh_defs.h"

#include "app_device.h"

#ifdef CONFIG_ESP_WROOM_32
#define LED_R GPIO_NUM_25
#define LED_G GPIO_NUM_26
#define LED_B GPIO_NUM_27
#elif defined(CONFIG_ESP_WROVER)
#define LED_R GPIO_NUM_0
#define LED_G GPIO_NUM_2
#define LED_B GPIO_NUM_23
#define SW_1  GPIO_NUM_33
#define SW_2  GPIO_NUM_32
#elif defined(CONFIG_ESP32C3_DEV)
#define BTN_NUM     1
#define LED_NUM     1
#define LED_R       GPIO_NUM_8
#define LED_G       GPIO_NUM_8
#define LED_B       GPIO_NUM_8
#define BTN_1       GPIO_NUM_9
#define SW_1        GPIO_NUM_2
#define SW_2        GPIO_NUM_4
#define SW_3        GPIO_NUM_5
#define SW_4        GPIO_NUM_6
#elif defined(CONFIG_ESP32C3_RADAR_DEV)
#define BTN_NUM     1
#define LED_NUM     1
#define LED_R       GPIO_NUM_3
#define LED_G       GPIO_NUM_3
#define LED_B       GPIO_NUM_2
#define BTN_1       GPIO_NUM_0
#define SW_1        GPIO_NUM_10
#define SW_2        GPIO_NUM_4
#define SW_3        GPIO_NUM_5
#define SW_4        GPIO_NUM_6
#elif defined(CONFIG_ESP32S3_DEV)
#define LED_R GPIO_NUM_47
#define LED_G GPIO_NUM_47
#define LED_B GPIO_NUM_47
#define SW_1  GPIO_NUM_33
#define SW_2  GPIO_NUM_32
#endif

#define LED_ON          0
#define LED_OFF         1
#define LED_FLIP        0
#define BTN_IDLE        1
#define BTN_PRESS       0
#define BTN_HOLD        2
#define BTN_HOLD_2      3
#define BTN_HOLD_3      4

#define SW_ON           1
#define SW_OFF          0
#define SW_INIT         2

struct _led_state {
    uint8_t current;
    uint8_t previous;
    uint8_t pin;
    char *name;
};

extern struct button_state{
    uint8_t element;
    uint8_t current;
    uint8_t previous;
    uint8_t pin;
    char *name;
} btn_state[BTN_NUM];

extern struct switch_state{
    uint8_t element;
    uint8_t current;
    uint8_t pin;
    bool set;
    char *name;
} sw_state[BTN_NUM];

void board_prov_complete(void);

void board_led_operation(uint8_t pin, uint8_t onoff);

esp_err_t board_init(void);

#ifdef __cplusplus
}
#endif

#endif
