#pragma once
#ifndef _APP_DEVICE_H_
#define _APP_DEVICE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "wile_init.h"
#include "wile_message.h"
#include "wile_device.h"

#define CTR_ONOFF_OFF  0
#define CTR_ONOFF_ON   1
#define CTR_ONOFF_FLIP 2
// extern bool localDevState;

typedef struct element_power_state{
    uint16_t element;
    uint8_t state;
} element_power_state_t;

extern uint8_t deviceLedFlipNum;
extern TaskHandle_t boardLedIndicateHardwareHandle;
extern TaskHandle_t boardSwMotorControlHandle;

void      root_device_state_init(void);
esp_err_t root_device_control(uint16_t element, uint16_t type, uint8_t *value);
esp_err_t root_device_local_control(uint8_t element, uint8_t mode);
void      root_device_identify(void);
void      root_device_delete_indicate(void);
void      root_device_prov_complete(void);
void      root_device_prov_none(void);
esp_err_t root_device_set_state(uint16_t element, uint16_t type, uint8_t *value);
esp_err_t root_device_get_state(uint16_t element, uint16_t feature, void **state, uint8_t *stateSize);

#ifdef __cplusplus
}
#endif

#endif