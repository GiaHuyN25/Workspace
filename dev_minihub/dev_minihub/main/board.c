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

#include <stdio.h>

#include "driver/gpio.h"
#include "board.h"

#define TAG "BOARD"

#if defined(CONFIG_ESP32C3_DEV) || defined(CONFIG_ESP32C3_RADAR_DEV)
struct _led_state led_state[3] = {
    { LED_OFF, LED_OFF, LED_R, "red"   },
    { LED_OFF, LED_OFF, LED_G, "green" },
    { LED_OFF, LED_OFF, LED_B, "blue"  },
};
struct button_state btn_state[BTN_NUM] = {
    { 1, BTN_IDLE, BTN_IDLE, BTN_1, "reset"  },
};
#endif

void board_led_operation(uint8_t pin, uint8_t onoff)
{
    #if defined(LED_RGB_DATA)

    #else
    for (int i = 0; i < 3; i++) {
        if (led_state[i].pin != pin) {
            continue;
        }
        if (onoff == led_state[i].previous) {
            ESP_LOGI(TAG, "led %s is already %s",
                     led_state[i].name, (onoff ? "on" : "off"));
            return;
        }
        gpio_set_level(pin, onoff);
        led_state[i].previous = onoff;
        return;
    }
    #endif

    ESP_LOGE(TAG, "LED is not found!");
}

TaskHandle_t btnTask = NULL;

static void button_task(void* arg)
{
    struct button_state *btn = (struct button_state *) arg;
    // ESP_LOGW("WILE", "Button Press: GPIO_%d, level: %d", btn->pin, gpio_get_level(btn->pin));
    vTaskDelay(50);
    if (gpio_get_level(btn->pin) != BTN_PRESS){
        btnTask = NULL;
        vTaskDelete(NULL);
    }

    uint8_t btnPressNum = 0;
    for (int i=0; i<BTN_NUM; i++){
        if (gpio_get_level(btn_state[i].pin) == BTN_PRESS){
            btn_state[i].current = BTN_PRESS;
            btnPressNum++;
        }
        else{
            btn_state[i].current = BTN_IDLE;
        }
    }

    #ifdef CONFIG_ESP32C3_WM_TY2_V05_4SW
    if (btn->pin == BTN_4){
        // esp_restart();
        root_device_factory_reset();
    }
    #endif

    for (int i=0; i<BTN_NUM; i++){
        if (btn_state[i].current == BTN_PRESS){
            ESP_LOGW("WILE", "Button Press: %s", btn_state[i].name);
            btn_state[i].current = BTN_IDLE;
        }
    }

    if (btnPressNum >= 1){
        TickType_t xLastWakeTime = xTaskGetTickCount();
        while (gpio_get_level(btn->pin) == BTN_PRESS){
            if(xTaskGetTickCount()-xLastWakeTime >= 3000){
                // root_device_factory_reset();
                for(int i=0; i<BTN_NUM; i++){
                    if(gpio_get_level(btn_state[i].pin) == BTN_PRESS){
                        if (btn_state[i].previous == BTN_HOLD){
                            ESP_LOGW("BOARD", "HOLD 2");
                            btn_state[i].current = BTN_HOLD_2;
                        }
                        else if (btn_state[i].previous == BTN_HOLD_2){
                            ESP_LOGW("BOARD", "HOLD 3");
                            btn_state[i].current = BTN_HOLD_3;
                        }
                        else{
                            ESP_LOGW("BOARD", "HOLD 1");
                            btn_state[i].current = BTN_HOLD;
                        }
                    }
                }
                break;
            }
            vTaskDelay(200);
        }
    }
    bool doReset = false;
    for(int i=0; i<BTN_NUM; i++){
        btn_state[i].previous = btn_state[i].current;
        if (btn_state[i].current >= BTN_HOLD) doReset = true;
    }
    if (doReset) root_device_factory_reset();
    btnTask = NULL;
    vTaskDelete(NULL);
}

static void IRAM_ATTR button_gpio_isr_handler(void* arg){
    struct button_state *btn = (struct button_state *) arg;
    if (gpio_get_level(btn->pin) == BTN_PRESS && btnTask == NULL){
        xTaskCreate(button_task, "button_task", 1024*3, (void *)btn, 10, &btnTask);
    }
}

static void board_led_init(void)
{
    // #if defined(CONFIG_ESP32C3_RD_CN_04_V11) || defined(CONFIG_ESP32C3_RD_CN_03_REM_V11)
    #ifdef LED_RGB_DATA
    board_led_rgb_init();
    #else
    for (int i = 0; i < 3; i++) {
        gpio_reset_pin(led_state[i].pin);
        gpio_set_direction(led_state[i].pin, GPIO_MODE_OUTPUT);
        gpio_set_level(led_state[i].pin, LED_OFF);
        led_state[i].previous = LED_OFF;
    }
    #endif
}

void board_button_init(void){
    gpio_config_t gpio_conf;
    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);

    for (int i=0; i<BTN_NUM; i++){
        // gpio_reset_pin(btn_state[i].pin);
        gpio_conf.intr_type = GPIO_INTR_NEGEDGE;
        gpio_conf.mode = GPIO_MODE_INPUT;
        gpio_conf.pin_bit_mask = (1ULL << btn_state[i].pin);
        gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        gpio_conf.pull_up_en = GPIO_PULLUP_ENABLE;
        gpio_config(&gpio_conf);
        // gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
        gpio_isr_handler_add(btn_state[i].pin, button_gpio_isr_handler, (void *)&(btn_state[i]));
    }
}

esp_err_t board_init(void){
    board_led_init();
    board_button_init();
    return ESP_OK;
}