#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "driver/gpio.h"
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_rom_gpio.h>
#include <seven_segment.h>
#include <gpio_init.h>
#include <esp_system.h>

const char *TAG = "Counting";

//Define each segment's pin
#define SEG_A           GPIO_NUM_2
#define SEG_B           GPIO_NUM_4
#define SEG_C           GPIO_NUM_16
#define SEG_D           GPIO_NUM_17
#define SEG_E           GPIO_NUM_5
#define SEG_F           GPIO_NUM_18
#define SEG_G           GPIO_NUM_19

//Define number position
#define D1              GPIO_NUM_27
#define D2              GPIO_NUM_26
#define D3              GPIO_NUM_25
#define D4              GPIO_NUM_33

static int ledPin[7]    =   {SEG_A, SEG_B, SEG_C, SEG_D, SEG_E, SEG_F, SEG_G};
static int Position[4]  =   {D1,D2,D3,D4};

//Counting
static int counting = 999;
static int input = 1500;

//Display function
void Number_Display(void * pvParameter);

//Counting Task
void CountTask(void * pvParameter);

void app_main() {
    //Initializating LED pin
    for (int i = 0; i < 7; i++)
    {
        output_init(*(ledPin+i), NO_INTR, OFF);
    }

    //Initializating LED position
    for (int i = 0; i<4; i++)
    {
        output_init(*(Position+i), NO_INTR, OFF);
    }

    ESP_LOGI(TAG, "%lu", esp_get_free_heap_size());

    xTaskCreate(Number_Display, "Number Display", 4096, NULL, 4, NULL);
    // xTaskCreate(Number1,"Number 1", 1024, NULL, 4, NULL);
    // xTaskCreate(Number2,"Number 1", 1024, NULL, 4, NULL);
    // xTaskCreate(Number3,"Number 1", 1024, NULL, 4, NULL);
    // xTaskCreate(Number4,"Number 1", 1024, NULL, 4, NULL);
    xTaskCreate(CountTask, "Counting Task", 1024*8, NULL, 8, NULL);

}

void Number_Display(void * pvParameter)
{
    while(1){
        numberDisplay_4_Segment(Position, counting, ledPin);
    }
}

void CountTask(void * pvParamter)
{
    while(1)
    {
        ESP_LOGI(TAG, "Counting to %d", counting);
        if(counting < input){
            counting += 1;
        }
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}