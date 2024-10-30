#include "driver/gpio.h"
#include "board.h"
#include "board_led.h"

led_strip_spi_t strip = LED_STRIP_SPI_DEFAULT();

const hsv_state_t hsv_init = { // BLUE
    .hue        = 240,
    .saturation = 100,
    .value      = 100,
};

#if defined(CONFIG_ESP32C3_RD_CN_04_V11)
struct led_state_rgb led_rgb_state[LED_NUM] = {
    { 2, 0, hsv_init, hsv_init, "BTN2_1" },
    { 2, 1, hsv_init, hsv_init, "BTN2_2" },
    { 1, 2, hsv_init, hsv_init, "BTN1_1" },
    { 1, 3, hsv_init, hsv_init, "BTN1_2" },
    { 3, 4, hsv_init, hsv_init, "BTN3_1" },
    { 3, 5, hsv_init, hsv_init, "BTN3_2" },
    { 4, 6, hsv_init, hsv_init, "BTN4_1" },
    { 4, 7, hsv_init, hsv_init, "BTN4_2" },
};
#elif defined(CONFIG_ESP32C3_RD_CN_03_V11)
struct led_state_rgb led_rgb_state[LED_NUM] = {
    { 1, 0, hsv_init, hsv_init, "BTN1_1" },
    { 1, 1, hsv_init, hsv_init, "BTN1_2" },
    { 2, 2, hsv_init, hsv_init, "BTN2_1" },
    { 2, 3, hsv_init, hsv_init, "BTN2_2" },
    { 3, 4, hsv_init, hsv_init, "BTN3_1" },
    { 3, 5, hsv_init, hsv_init, "BTN3_2" },
};
#elif defined(CONFIG_ESP32C3_RD_CN_03_REM_V11)
struct led_state_rgb led_rgb_state[LED_NUM] = {
    { 1, 0, hsv_init, hsv_init, "OPEN_1"  },
    { 1, 1, hsv_init, hsv_init, "OPEN_2"  },
    { 2, 2, hsv_init, hsv_init, "STOP_1"  },
    { 2, 3, hsv_init, hsv_init, "STOP_2"  },
    { 3, 4, hsv_init, hsv_init, "CLOSE_1" },
    { 3, 5, hsv_init, hsv_init, "CLOSE_2" },
};
#endif

bool boardledFlipDone = true;

void board_led_rgb_init(void){
    esp_err_t err = led_strip_spi_install();
    if (err != ESP_OK) {
        ESP_LOGE("LED", "led_strip_spi_install(): %s", esp_err_to_name(err));
    }

    strip.length = LED_NUM;
    strip.mosi_io_num = GPIO_NUM_3;
    strip.sclk_io_num = GPIO_NUM_10;
    static spi_device_handle_t device_handle;
    strip.device_handle = device_handle;
    strip.max_transfer_sz = LED_STRIP_SPI_BUFFER_SIZE(LED_NUM);
    strip.clock_speed_hz = 1000000 * 10; // 10Mhz

    ESP_LOGI(TAG, "Initializing LED strip");
    ESP_ERROR_CHECK(led_strip_spi_init(&strip));
}

// uint32_t led_effect_color_wheel(uint8_t pos){
//     pos = 255 - pos;
//     if (pos < 85){
//         return ((uint32_t)(255 - pos * 3) << 16) | ((uint32_t)(0) << 8) | (pos * 3);
//     }
//     else if (pos < 170){
//         pos -= 85;
//         return ((uint32_t)(0) << 16) | ((uint32_t)(pos * 3) << 8) | (255 - pos * 3);
//     }
//     else{
//         pos -= 170;
//         return ((uint32_t)(pos * 3) << 16) | ((uint32_t)(255 - pos * 3) << 8) | (0);
//     }
// }

// rgb_t led_effect_color_wheel_rgb(uint8_t pos){
//     uint32_t next_color;
//     rgb_t next_pixel;

//     next_color = led_effect_color_wheel(pos);
//     next_pixel.r = (next_color >> 16) & 0xff;
//     next_pixel.g = (next_color >>  8) & 0xff;
//     next_pixel.b = (next_color      );
//     return next_pixel;
// }

// static esp_err_t simple_rgb(led_strip_spi_t *strip){
//     static uint8_t counter = 0;
//     uint32_t c;
//     rgb_t color;
//     esp_err_t err;

//     c = 0x0000a0 << ((counter % 3) * 8);
//     color.r = (c >> 16) & 0xff;
//     color.g = (c >> 8)  & 0xff;
//     color.b =  c        & 0xff;
//     ESP_LOGI(TAG, "r: 0x%02x g: 0x%02x b: 0x%02x", color.r, color.g, color.b);

//     if ((err = led_strip_spi_fill(strip, 0, strip->length, color)) != ESP_OK) {
//         ESP_LOGE(TAG, "led_strip_spi_fill(): %s", esp_err_to_name(err));
//         goto fail;
//     }
//     counter += 1;
// fail:
//     return err;
// }

// static esp_err_t rainbow_scroll(led_strip_spi_t *strip){
//     static uint8_t pos = 0;
//     const uint8_t offset = 0xff / LED_NUM >= 1 ? 0xff / LED_NUM : 1;

//     esp_err_t err = ESP_FAIL;
//     rgb_t color;elements", esp_err_to_name(err));
//             goto fail;
//         }
//     }
//     // ESP_LOGI(TAG, "pos: %d", pos);
//     pos += 1;
// fail:
//     return err;

// }

rgb_t hsv2rgb(hsv_state_t hsv) {
	float r = 0, g = 0, b = 0;
	
	float h = (float)hsv.h / 360;
	float s = (float)hsv.s / 100;
	float v = (float)hsv.v / 100;
	
	int i = floor(h * 6);
	float f = h * 6 - i;
	float p = v * (1 - s);
	float q = v * (1 - f * s);
	float t = v * (1 - (1 - f) * s);
	
	switch (i % 6) {
		case 0: r = v, g = t, b = p; break;
		case 1: r = q, g = v, b = p; break;
		case 2: r = p, g = v, b = t; break;
		case 3: r = p, g = q, b = v; break;
		case 4: r = t, g = p, b = v; break;
		case 5: r = v, g = p, b = q; break;
	}
	
	rgb_t color;
	color.r = r * 255 * LED_RED_RATIO;
	color.g = g * 255 * LED_GREEN_RATIO;
	color.b = b * 255 * LED_BLUE_RATIO;
	
	return color;
}

void board_led_rgb_set(int8_t element, uint16_t color, uint8_t brightness){
    /* turn off all LEDs */
    ESP_ERROR_CHECK(led_strip_spi_flush(&strip));
    esp_err_t err = ESP_OK;

    for (uint8_t i=0; i<LED_NUM; i++){
        if (led_rgb_state[i].element == element){
            led_rgb_state[i].previous = led_rgb_state[i].current;
            led_rgb_state[i].current.hue = color;
            led_rgb_state[i].current.value = brightness;
            rgb_t rgb = hsv2rgb(led_rgb_state[i].current);
            // if ((err = led_strip_spi_set_pixel(&strip, led_rgb_state[i].index, rgb)) != ESP_OK) {
            if ((err = led_strip_spi_set_pixel_brightness(&strip, led_rgb_state[i].index, rgb, LED_BRIGHTNESS_PERCENT)) != ESP_OK) {
                ESP_LOGE(TAG, "led_strip_spi_fill(): %s", esp_err_to_name(err));
            }
        }
    }
    ESP_ERROR_CHECK(led_strip_spi_flush(&strip));
}

void board_led_rgb_set_color(int8_t element, uint16_t color, bool set){
    ESP_LOGI("LED", "board_led_rgb_set_color");
    esp_err_t err = ESP_OK;
    if (element == LED_ALL){ // ALL
        rgb_t rgb = hsv2rgb(led_rgb_state[0].current);
        for (uint8_t i=0; i<LED_NUM; i++){
            led_rgb_state[i].previous = led_rgb_state[i].current;
            led_rgb_state[i].current.hue = color;
            rgb = hsv2rgb(led_rgb_state[i].current);
            // if ((err = led_strip_spi_set_pixel(&strip, led_rgb_state[i].index, rgb)) != ESP_OK) {
            if ((err = led_strip_spi_set_pixel_brightness(&strip, led_rgb_state[i].index, rgb, LED_BRIGHTNESS_PERCENT)) != ESP_OK) {
                ESP_LOGE(TAG, "board_led_rgb_set_color: %s", esp_err_to_name(err));
            }
        }
        if (!set) return;
        // rgb_t rgb = hsv2rgb(led_rgb_state[i].current);
        // ESP_ERROR_CHECK(led_strip_spi_flush(&strip));
        // if ((err = led_strip_spi_fill(&strip, 0, strip.length, rgb)) != ESP_OK) {
        //     ESP_LOGE(TAG, "led_strip_spi_fill(): %s", esp_err_to_name(err));
        // }
        ESP_ERROR_CHECK(led_strip_spi_flush(&strip));
    }
    else{
        for (uint8_t i=0; i<LED_NUM; i++){
            if (led_rgb_state[i].element == element){
                led_rgb_state[i].previous = led_rgb_state[i].current;
                led_rgb_state[i].current.hue = color;
                // if (color == LED_YELLOW) led_rgb_state[i].current.value *= LED_YELLOW_RATIO;
                if (!set) continue;
                rgb_t rgb = hsv2rgb(led_rgb_state[i].current);
                // if ((err = led_strip_spi_set_pixel(&strip, led_rgb_state[i].index, rgb)) != ESP_OK) {
                if ((err = led_strip_spi_set_pixel_brightness(&strip, led_rgb_state[i].index, rgb, LED_BRIGHTNESS_PERCENT)) != ESP_OK) {
                    ESP_LOGE(TAG, "board_led_rgb_set_color: %s", esp_err_to_name(err));
                }
                ESP_ERROR_CHECK(led_strip_spi_flush(&strip));
            }
        }
    }
}

void board_led_rgb_set_brightness(int8_t element, uint8_t brightness){
    // ESP_LOGW("LED", "board_led_rgb_set_brightness, elm: %d, brightness: %d", element, brightness);
    esp_err_t err = ESP_OK;
    if (element == LED_ALL){ // ALL
        rgb_t rgb = hsv2rgb(led_rgb_state[0].current);
        for (uint8_t i=0; i<LED_NUM; i++){
            led_rgb_state[i].previous = led_rgb_state[i].current;
            led_rgb_state[i].current.value = brightness;
            rgb = hsv2rgb(led_rgb_state[i].current);
            // if ((err = led_strip_spi_set_pixel(&strip, led_rgb_state[i].index, rgb)) != ESP_OK) {
            if ((err = led_strip_spi_set_pixel_brightness(&strip, led_rgb_state[i].index, rgb, LED_BRIGHTNESS_PERCENT)) != ESP_OK) {
                ESP_LOGE(TAG, "board_led_rgb_set_brightness: %s", esp_err_to_name(err));
            }
        }
        // ESP_ERROR_CHECK(led_strip_spi_flush(&strip));
        // if ((err = led_strip_spi_fill(&strip, 0, strip.length, rgb)) != ESP_OK) {
        //     ESP_LOGE(TAG, "led_strip_spi_fill(): %s", esp_err_to_name(err));
        // }
        ESP_ERROR_CHECK(led_strip_spi_flush(&strip));
    }
    else{
        for (uint8_t i=0; i<LED_NUM; i++){
            if (led_rgb_state[i].element == element){
                led_rgb_state[i].previous = led_rgb_state[i].current;
                if (brightness == LED_FLIP){
                    if (led_rgb_state[i].current.value == LED_ON) led_rgb_state[i].current.value = LED_OFF;
                    if (led_rgb_state[i].current.value == LED_OFF) led_rgb_state[i].current.value = LED_ON;
                }
                else{
                    led_rgb_state[i].current.value = brightness;
                }
                // if (led_rgb_state[i].current.hue == LED_YELLOW)
                //     led_rgb_state[i].current.value *= LED_YELLOW_RATIO;
                rgb_t rgb = hsv2rgb(led_rgb_state[i].current);
                // if ((err = led_strip_spi_set_pixel(&strip, led_rgb_state[i].index, rgb)) != ESP_OK) {
                if ((err = led_strip_spi_set_pixel_brightness(&strip, led_rgb_state[i].index, rgb, LED_BRIGHTNESS_PERCENT)) != ESP_OK) {
                    ESP_LOGE(TAG, "board_led_rgb_set_brightness: %s", esp_err_to_name(err));
                }
                ESP_ERROR_CHECK(led_strip_spi_flush(&strip));
            }
        }
    }
}

void board_led_rgb_set_saturation(int8_t element, uint8_t saturation){
    esp_err_t err = ESP_OK;
    if (element == LED_ALL){ // ALL
        rgb_t rgb = hsv2rgb(led_rgb_state[0].current);
        for (uint8_t i=0; i<LED_NUM; i++){
            led_rgb_state[i].previous = led_rgb_state[i].current;
            led_rgb_state[i].current.saturation = saturation;
            rgb = hsv2rgb(led_rgb_state[i].current);
            // if ((err = led_strip_spi_set_pixel(&strip, led_rgb_state[i].index, rgb)) != ESP_OK) {
            if ((err = led_strip_spi_set_pixel_brightness(&strip, led_rgb_state[i].index, rgb, LED_BRIGHTNESS_PERCENT)) != ESP_OK) {
                ESP_LOGE(TAG, "board_led_rgb_set_saturation: %s", esp_err_to_name(err));
            }
        }
        ESP_ERROR_CHECK(led_strip_spi_flush(&strip));
    }
    else{
        for (uint8_t i=0; i<LED_NUM; i++){
            if (led_rgb_state[i].element == element){
                led_rgb_state[i].previous = led_rgb_state[i].current;
                led_rgb_state[i].current.saturation = saturation;
                rgb_t rgb = hsv2rgb(led_rgb_state[i].current);
                // if ((err = led_strip_spi_set_pixel(&strip, led_rgb_state[i].index, rgb)) != ESP_OK) {
                if ((err = led_strip_spi_set_pixel_brightness(&strip, led_rgb_state[i].index, rgb, LED_BRIGHTNESS_PERCENT)) != ESP_OK) {
                    ESP_LOGE(TAG, "board_led_rgb_set_saturation: %s", esp_err_to_name(err));
                }
                ESP_ERROR_CHECK(led_strip_spi_flush(&strip));
            }
        }
    }
}

void board_led_rgb_flip_task(void *pvParameters){
    uint8_t flipNum = *((uint8_t *)pvParameters);
    boardledFlipDone = false;
    // ESP_LOGW("board_led_rgb_flip_task", "num: %d", flipNum);
    for (uint8_t i=0; i<flipNum; i++){
        board_led_rgb_set_brightness(LED_ALL, LED_ON);
        vTaskDelay(200);
        board_led_rgb_set_brightness(LED_ALL, LED_OFF);
        vTaskDelay(200);
    };
    for (uint8_t i=0; i<BTN_NUM; i++){
        if (sw_state[i].current == CTR_ONOFF_ON){
            board_led_rgb_set_brightness(sw_state[i].element, LED_ON);
        }
        else{
            board_led_rgb_set_brightness(sw_state[i].element, LED_OFF);
        }
    }
    boardledFlipDone = true;
    vTaskDelete(NULL);
}

void board_led_rgb_prov_none_task(void *pvParameters){
    while(PROV_STATE != STEP_CFG_COMPLETE){
        // ESP_LOGW("BOARD", "board_led_rgb_prov_none_task");
        while (!boardledFlipDone) vTaskDelay(1);
        deviceLedFlipNum = 2;
        xTaskCreate(board_led_rgb_flip_task, "board_led_rgb_flip_task", 1024, &deviceLedFlipNum, tskIDLE_PRIORITY, NULL);
        vTaskDelay(600*1000);
        // vTaskDelay(2000);
    }
    vTaskDelete(ledIndicateHandle);
}

void board_led_rgb_prov_run_task(void *pvParameters){
    deviceLedFlipNum = 10;
    xTaskCreate(board_led_rgb_flip_task, "board_led_rgb_flip_task", 1024, &deviceLedFlipNum, tskIDLE_PRIORITY, NULL);

    if (ledIndicateHandle != NULL){
        vTaskResume(ledIndicateHandle);
    }
    vTaskDelete(NULL);
}

void board_led_rgb_wifi_drop_task(void *pvParameters){
    while(1){
        // ESP_LOGW("BOARD", "board_led_rgb_wifi_drop_task");
        while (!boardledFlipDone) vTaskDelay(1);
        deviceLedFlipNum = 2;
        xTaskCreate(board_led_rgb_flip_task, "board_led_rgb_flip_task", 1024, &deviceLedFlipNum, tskIDLE_PRIORITY, NULL);
        vTaskDelay(300*1000);
        // vTaskDelay(1000);
    }
    vTaskDelete(NULL);
}

void board_led_rgb_hardware_fail_task(void *pvParameters){
    while(1){
        // ESP_LOGW("BOARD", "board_led_rgb_wifi_drop_task");
        while (!boardledFlipDone) vTaskDelay(1);
        deviceLedFlipNum = 2;
        xTaskCreate(board_led_rgb_flip_task, "board_led_rgb_flip_task", 1024, &deviceLedFlipNum, tskIDLE_PRIORITY, NULL);
        vTaskDelay(3*1000);
    }
    vTaskDelete(NULL);
}