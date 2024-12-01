#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "gpio_init.h"

const char *ADC_TAG = "Analog Read";

#define HW038_DATA      ADC_CHANNEL_4

#define MAX_WATER_HEIGHT 4.3        // Chiều cao nước tối đa (cm)
#define MIN_VOLTAGE 330             // Điện áp ở mức nước thấp nhất
#define MAX_VOLTAGE 2000            // Điện áp ở mức nước cao nhất

static int hw038_raw[10];

float calculate_water_height(float voltage) {
    if (voltage < MIN_VOLTAGE) return 0.0;
    if (voltage > MAX_VOLTAGE) return MAX_WATER_HEIGHT;
    
    float water_height = ((voltage - MIN_VOLTAGE) / 
                          (MAX_VOLTAGE - MIN_VOLTAGE)) * MAX_WATER_HEIGHT;
    
    return water_height;
}

void app_main(void)
{
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t adc_init_config = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_init_config, &adc_handle));

    adc_oneshot_chan_cfg_t adc_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, HW038_DATA, &adc_config));
    
    float hw038_h; 

    while(1)
    {
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, HW038_DATA, &hw038_raw[0]));
        ESP_LOGI(ADC_TAG, "HW - 038 Raw Data: %d mV.", hw038_raw[0]);
        hw038_h = calculate_water_height((float) hw038_raw[0]);
        ESP_LOGI(ADC_TAG, "HW - 038 Height: %.2f cm.", hw038_h);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    

}