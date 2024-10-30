#ifdef __cplusplus
extern "C" {
#endif
#include "board.h"
#include "board_led.h"
#include "app_device.h"
#include "wile_define.h"

#if defined(CONFIG_ESP32C3_DEV) || defined(CONFIG_ESP32C3_RADAR_DEV)
element_power_state_t elementState[BTN_NUM] = {
    { 1, CTR_ONOFF_OFF },
};
uint8_t deviceLedFlipNum = 2;
TaskHandle_t boardLedIndicateHardwareHandle = NULL;
uint8_t deviceZeroCrossingCheck = true;
#else
element_power_state_t elementState[BTN_NUM] = {
    { 1, CTR_ONOFF_OFF },
    { 2, CTR_ONOFF_OFF },
    { 3, CTR_ONOFF_OFF },
    { 4, CTR_ONOFF_OFF },
};
uint8_t deviceLedFlipNum = 2;
TaskHandle_t boardLedIndicateHardwareHandle = NULL;
#endif

void root_device_state_init(void){
}

esp_err_t root_device_control(uint16_t element, uint16_t type, uint8_t *value){
    ESP_LOGI("DEVICE", "Root device control, elm: %d", element);
    ESP_LOG_BUFFER_HEX("ATTR VALUE", value, rgmsg_feature_size(type));
    return ESP_OK;
}

esp_err_t root_device_local_control(uint8_t element, uint8_t mode){
    return ESP_OK;
}

#ifndef LED_RGB_DATA
static void task_led_slow(void *pvParameters)
{
    while(PROV_STATE != STEP_CFG_COMPLETE){
        vTaskDelay(500);
        gpio_set_level(LED_B, LED_OFF);
        vTaskDelay(500);
        gpio_set_level(LED_B, LED_ON);
    }
    vTaskDelete(ledIndicateHandle);
}

static void task_led_fast(void *pvParameters)
{
    uint32_t lastTime = xTaskGetTickCount();
    while(xTaskGetTickCount() - lastTime <= 5000){
        vTaskDelay(100);
        gpio_set_level(LED_R, LED_OFF);
        vTaskDelay(100);
        gpio_set_level(LED_R, LED_ON);
    }
    if (ledIndicateHandle != NULL){
        vTaskResume(ledIndicateHandle);
    }
    vTaskDelete(NULL);
}
#endif

void root_device_identify(void){
    #ifdef LED_RGB_DATA
    xTaskCreate(board_led_rgb_prov_run_task, "board_led_rgb_prov_run_task", 512, NULL, tskIDLE_PRIORITY, NULL);
    #endif
}

esp_err_t rgmgt_device_event_cb(uint16_t event){
    switch (event){
        case CTR_WILE_EVT_WIFI_CONNECTED:
            break;
        case CTR_WILE_EVT_WIFI_DISCONNECTED:
            break;
        case CTR_WILE_EVT_WIFI_CONNECT_FAIL:
            break;
        case CTR_WILE_EVT_CLOUD_CONNECTED:
        case CTR_WILE_EVT_DEVICE_CONTROL_DONE:
            gpio_set_level(LED_B, LED_OFF);
            break;
        case CTR_WILE_EVT_CLOUD_DISCONNECTED:
        case CTR_WILE_EVT_DEVICE_CONTROL_START:
            gpio_set_level(LED_B, LED_ON);
            break;
        default:
            break;
    }
    return ESP_OK;
}

void root_device_delete_indicate(void){
    #ifdef LED_RGB_DATA
    deviceLedFlipNum = 4;
    xTaskCreate(board_led_rgb_flip_task, "board_led_rgb_flip_task", 1024, &deviceLedFlipNum, tskIDLE_PRIORITY, NULL);
    #endif
}

void root_device_prov_complete(void){
    gpio_set_level(LED_B, LED_OFF);
}

void root_device_prov_none(void){
    xTaskCreate(task_led_slow, "task_led_slow", 512, NULL, tskIDLE_PRIORITY, &ledIndicateHandle);
}

esp_err_t root_device_set_state(uint16_t element, uint16_t type, uint8_t *value){
    return rgmgt_device_set_state(rootEID, element, type, value, true, true);
}

esp_err_t root_device_get_state(uint16_t element, uint16_t feature, void **state, uint8_t *stateSize){
    esp_err_t ret = ESP_OK;
    uint8_t *elmInfo = NULL;
    size_t elmInfoLen = 0;
    ret = rgmgt_device_get_state(rootEID, (void **)&elmInfo, &elmInfoLen);
    ESP_LOG_BUFFER_HEX("STATE", elmInfo, elmInfoLen);

    uint8_t *elmState = NULL;
    size_t elmStateLen = 0;
    ret += rgmgt_device_get_state_elm_attr(element, feature, elmInfo, elmInfoLen, (void **)&elmState, &elmStateLen);
    *stateSize = elmStateLen;
    if (*state != NULL) free(*state);
    *state = malloc(*stateSize);
    memcpy(*state, elmState, *stateSize);

    if (elmInfo != NULL) free(elmInfo);
    if (elmState != NULL) free(elmState);
    return ret;
}

#ifdef __cplusplus
}
#endif