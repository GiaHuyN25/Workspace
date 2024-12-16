#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

#include <driver/gpio.h>
#include <driver/uart.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_err.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_mac.h"
#include "esp_sntp.h"
#include "esp_netif_sntp.h"
#include "esp_task_wdt.h"

#include "lwip/inet.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "sdkconfig.h"

#include "wifi_lib.h"
#include "web_server_app.h"


static char *WIFI_TAG = "Wifi Station Mode";
static char *UART_TAG = "UART Event";

//Define UART pin
#define UART_DATA_PORT      UART_NUM_1                      //UART port
#define UART_DATA_TX        GPIO_NUM_36                      //Data UART Transmit GPIO
#define UART_DATA_RX        GPIO_NUM_37                      //Data UART Receive GPIO
#define BUFFER              1024                            //Buffer length unit

#define UART_WIFI_PORT      UART_NUM_2                      //UART port
#define UART_WIFI_TX        GPIO_NUM_17                      //Data UART Transmit GPIO
#define UART_WIFI_RX        GPIO_NUM_18                     //Data UART Receive GPIO

static QueueHandle_t uart_data_event;
static QueueHandle_t uart_wifi_event;


//Wifi
static EventGroupHandle_t s_wifi_event_group;

static char SSID[30] = "Phong301_an_trai";
static char PASSWORD[30] = "12344321";

static esp_event_handler_instance_t instance_any_id;        
static esp_event_handler_instance_t instance_got_ip;

static int retry_count = 0;
static int wifi_state = 0;                                  //Wifi connected or not connected


//HTTP Protocol
static EventGroupHandle_t s_http_event_group;

#define HTTP_UPDATE_DATA_BIT        BIT0

static char REQUEST[512];                                   //HTTP Request string
static char HTTP_RECV[512];                                 //HTTP Receive string

//Sensor Data receive from Arduino
int hour = 0;
int minute = 0;
int temperature = 0;
int humidity = 0;
int water_volume = 0;
int solidHumidity = 0;
int plantStatus = 1;

//User Mode
int ManualMode = 0;
int MotorPWM = 0;
int MotorPump = 0;
int Lamp = 0;

//Wifi functions
void Wifi_Task(void *pvParameter);                          //Report about Wifi connecting status
void event_handler(void* arg, esp_event_base_t event_base,
                    int32_t event_id, void* event_data);    //Wifi connect events


//UART functions
static EventGroupHandle_t s_uart_mode_event_group; 

#define UART_MANUAL_MODE_ON     BIT0
#define UART_MANUAL_MODE_OFF    BIT1

void UART1_Config();                                        //UART Configuring
void UART2_Config();

void UART_Data_Task(void *pvParameter);                          //UART Receive Data

void UARTString_Analyze(char *data);
void dataExtract(char *subString, char *String, int *value);
void UART_data_handle();
void MotorPWM_control();
void MotorPump_control();
void Lamp_control();

void UART_Wifi_String_Analyze(char *data);
void UART_Wifi_Task(void *pvParameter);
ssid_valid_t Wifi_Info_Extract(char *subString, char *String,
                        char *currentString);
ssid_valid_t Wifi_Info_Confirm(char *String);

ssid_valid_t ssid_err = 0;
ssid_valid_t pass_err = 0;
ssid_valid_t confirm_err = 0;



//HTTP Functions
void http_task(void *pvParameter);                          //HTTP event when an event occurs
void http_handle();                                         //HTTP initiate socket and send request
void http_request(int s, int *r);                           //Send HTTP request

//Web_server
void esp32cam_callback(char *data, int len);
void switch_data_callback(char *data, int len);
void motor_mode_callback(char *data, int len);
void get_data_callback();

void UART_slave_mode_send(void *pvParameter);

void app_main(void)
{
    //Initialize NVS for wifi station mode
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    esp_task_wdt_deinit();


    //UART and Wifi Group Event Initializing
    s_wifi_event_group = xEventGroupCreate();
    s_http_event_group = xEventGroupCreate();
    s_uart_mode_event_group = xEventGroupCreate();

    UART1_Config();
    UART2_Config();
    
    wifi_init_sta(SSID, PASSWORD, &event_handler, &instance_any_id, &instance_got_ip);
    start_webserver();

    http_set_callback_switch(switch_data_callback);
    http_set_callback_get_data(get_data_callback);
    http_set_callback_motor(motor_mode_callback);
    http_set_callback_esp32cam(esp32cam_callback);

    //Task Create
    xTaskCreate(UART_Wifi_Task, "UART Wifi Task", 16*1024, NULL, 14, NULL);

    xTaskCreate(UART_slave_mode_send, "UART to Arduino", 10*1024, NULL, 13, NULL);
    
    xTaskCreate(Wifi_Task, "Wifi Task", 10*1024, NULL, 12, NULL);

    xTaskCreate(UART_Data_Task, "UART Wifi Task", 16*1024, NULL, 11, NULL);

    xTaskCreate(http_task, "HTTP Task", 10*1024, NULL, 10, NULL);

    

}

void UART1_Config()
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .parity = UART_PARITY_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
        .stop_bits = UART_STOP_BITS_1,
    };

    uart_driver_install(UART_DATA_PORT, BUFFER*2, BUFFER*2, 20, &uart_data_event, NULL);
    uart_param_config(UART_DATA_PORT, &uart_config);
    uart_set_pin(UART_DATA_PORT, UART_DATA_TX, UART_DATA_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

void UART2_Config()
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .parity = UART_PARITY_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
        .stop_bits = UART_STOP_BITS_1,
    };

    uart_driver_install(UART_WIFI_PORT, BUFFER*2, BUFFER*2, 20, &uart_wifi_event, NULL);
    uart_param_config(UART_WIFI_PORT, &uart_config);
    uart_set_pin(UART_WIFI_PORT, UART_WIFI_TX, UART_WIFI_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

void dataExtract(char *subString, char *String, int *value)
{
    char number[7];
    int temp = *value;
    char *pos = strstr((char *) String, subString);
    if (!pos) {
        //Empty to skip function
    }
    else
    {
        int i = 0;
        pos += strlen(subString);  
        // while (*(pos + i) != ' ') 
        while(isdigit((int) *(pos + i)))
        {  
            number[i] = *(pos + i);
            i++;
        }
        if(i != 0)
        {
            number[i] = '\0';
            *value = atoi(number);
        }
    }
}

ssid_valid_t Wifi_Info_Extract(char *subString, char *String, char *currentString)
{
    int i = 0;
    char *pos = strstr( String, subString);
    if (!pos) {
        return NOCHANGE;
    }

    pos += strlen(subString);
    while (*(pos + i) != ' ' && *(pos + i) != '\0') 
    {
        if (i >= 29) return INVALID;
        *(currentString + i) = *(pos + i);
        i ++;
    }
    if(i == 0) return VALID;
    *(currentString + i) = '\0';
    return VALID;
}

ssid_valid_t Wifi_Info_Confirm(char *String)
{
    int i = 0;
    char *pos = strstr((char *) String, "Yes");
    if (!pos) {
        return NOCHANGE;
    }

    return CONFIRM;
}

void UARTString_Analyze(char *data)
{
    dataExtract("Hour: ", data, &hour);
    dataExtract("Minute: ", data, &minute);
    dataExtract("Temperature: ", data, &temperature);
    dataExtract("Humidity: ", data, &humidity);
    dataExtract("Solid Humidity: ", data, &solidHumidity);
    dataExtract("Water Volume: ", data, &water_volume);

    char data_after_extract[512];
    sprintf(data_after_extract, "Hour: %d , Minute: %d , Temperature: %d , Humidity: %d\
 , Solid Humidity: %d , Water Volume: %d .", 
                                hour, minute, temperature, humidity, water_volume, solidHumidity);
    printf("%s\n", data_after_extract);
}

void UART_data_handle()
{
    if(ManualMode == 0)
    {
        printf("Not Manual Mode.\n");
        MotorPWM_control();
        MotorPump_control();
        Lamp_control();
        char slave_mode_set[128];
        // sprintf(slave_mode_set, "Motor PWM: %d , Motor Pump: %d , Lamp: %d .", MotorPWM, MotorPump, Lamp);
        sprintf(slave_mode_set, "Motor PWM: %d .", MotorPWM);
        printf("%s\n", slave_mode_set);
        uart_write_bytes(UART_DATA_PORT, slave_mode_set, strlen(slave_mode_set));
        memset(slave_mode_set, 0, strlen(slave_mode_set));
        vTaskDelay(500 / portTICK_PERIOD_MS);
        sprintf(slave_mode_set, "Motor Pump: %d .", MotorPump);
        printf("%s\n", slave_mode_set);
        uart_write_bytes(UART_DATA_PORT, slave_mode_set, strlen(slave_mode_set));
        memset(slave_mode_set, 0, strlen(slave_mode_set));
        vTaskDelay(500 / portTICK_PERIOD_MS);
        sprintf(slave_mode_set, "Lamp: %d .", Lamp);
        printf("%s\n", slave_mode_set);
        uart_write_bytes(UART_DATA_PORT, slave_mode_set, strlen(slave_mode_set));
        memset(slave_mode_set, 0, strlen(slave_mode_set));
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void MotorPWM_control()
{
    if (temperature >= 31)          // HIGH Motor PWM condition
    {
        MotorPWM = 33;
    }
    else if (temperature >= 29)     // MED Motor PWM condition
    {
        MotorPWM = 67;
    }
    else if (temperature >= 27)     // LOW Motor PWM condition
    {
        MotorPWM = 100;
    }
    else                            // OFF Motor PWM condition
    {
        MotorPWM = 0;
    }

    if(humidity > 85)
    {
        MotorPWM = 75;
    }
}

void MotorPump_control()
{
    if (humidity < 55 && solidHumidity < 80)         //ON Pump condition
    {
        MotorPump = 1;
    }

    if (humidity > 75)                               //OFF Pump condition
    {
        MotorPump = 0;
    }

    if (solidHumidity < 55)                     
    {
        MotorPump = 1;
    }

    if ((hour == 9 || hour == 13 ) && (minute < 3 && humidity < 80 && solidHumidity < 80))
    {
        MotorPump = 1;
    }

    if(water_volume < 500)                               //Force OFF when water volume is low
    {
        MotorPump = 0;
    }
}

void Lamp_control()
{
    if (temperature <= 20)                 //ON Lamp condition 
    {
        Lamp = 1;
    }
    if (temperature >= 25)                 //OFF Lamp condition
    {
        Lamp = 0;
    }
}

void UART_Wifi_String_Analyze(char *data)
{
    ssid_err = Wifi_Info_Extract("SSID: ", data, SSID);
    pass_err = Wifi_Info_Extract("PASSWORD: ", data, PASSWORD);
    confirm_err = 1;

}

void Wifi_Info_Validication(ssid_valid_t validication)
{
    switch (validication)
    {
        case NOCHANGE:
            break;

        case VALID:
            char buffer[128];
            int len = snprintf(buffer, sizeof(buffer), "SSID: %s, Password: %s.\n", SSID, PASSWORD);
            if (len > 0 && len < sizeof(buffer)) {
                uart_write_bytes(UART_WIFI_PORT, buffer, len);
            }
            uart_write_bytes(UART_WIFI_PORT, "Enter \"Yes\" to change Wifi.\n", 29);
            break;

        case INVALID:
            uart_write_bytes(UART_WIFI_PORT, "Invalid SSID or PASSWORD. Enter again.\n", 40);
            memset(SSID, 0, 30);
            memset(PASSWORD, 0, 30);
            break;

        case CONFIRM:
            wifi_state = 0;
            wifi_cleanup(instance_any_id, instance_got_ip);
            wifi_init_sta(SSID, PASSWORD, &event_handler, &instance_any_id, &instance_got_ip);
            confirm_err = NOCHANGE;
            break;
    
    default:
        break;
    }
}

void UART_Data_Task(void *pvParameter)
{
    uart_event_t data_event;
    while(1)
    {
        if(xQueueReceive(uart_data_event, (void *)&data_event, (TickType_t)portMAX_DELAY))
        {
            uint8_t *data = (uint8_t *) malloc(BUFFER);
            ESP_LOGI(UART_TAG, "UART num %d.", UART_DATA_PORT);
            switch(data_event.type)
            {
                case UART_DATA:
                    ESP_LOGI("UART", "[UART DATA]: %d", data_event.size);
                    uart_read_bytes(UART_DATA_PORT, data, data_event.size, portMAX_DELAY);
                    ESP_LOGI("UART", "%s", data);
                    UARTString_Analyze((char *) data);
                    xEventGroupSetBits(s_http_event_group, HTTP_UPDATE_DATA_BIT);
                    UART_data_handle();
                    break;
                case UART_FIFO_OVF:
                ESP_LOGI("UART:", "hw fifo overflow");
                    uart_flush_input(UART_DATA_PORT);
                    xQueueReset(uart_data_event);
                    break;
                case UART_BUFFER_FULL:
                    uart_flush_input(UART_DATA_PORT);
                    xQueueReset(uart_data_event);
                    break;
                case UART_BREAK:
                    ESP_LOGI("UART:", "uart rx break");
                    break;
                case UART_PARITY_ERR:
                    ESP_LOGI("UART:", "uart parity error");
                    break;
                case UART_FRAME_ERR:
                    ESP_LOGI("UART:", "uart frame error");
                    break;
                default:
                    ESP_LOGI("UART:", "uart data_event type: %d", data_event.type);
                break;
            }
        free(data);
        data = NULL;
            
        }
    }
    vTaskDelete(NULL);
}

void UART_Wifi_Task(void *pvParameter)
{
    uart_event_t wifi_event;
    while(1)
    {
        if(xQueueReceive(uart_wifi_event, (void *)&wifi_event, (TickType_t)portMAX_DELAY))
        {
            uint8_t *data = (uint8_t *) malloc(BUFFER);
            ESP_LOGI(UART_TAG, "UART num %d.", UART_WIFI_PORT);
            switch(wifi_event.type)
            {
                case UART_DATA:
                    ESP_LOGI("UART", "[UART DATA]: %d", wifi_event.size);
                    uart_read_bytes(UART_WIFI_PORT, data, wifi_event.size, portMAX_DELAY);
                    ESP_LOGI("UART", "%s", data);
                    UART_Wifi_String_Analyze((char *) data);
                    Wifi_Info_Validication(ssid_err);
                    Wifi_Info_Validication(pass_err);
                    confirm_err = Wifi_Info_Confirm((char *)data);
                    Wifi_Info_Validication(confirm_err);
                    printf("%s\n", data);
                    break;
                case UART_FIFO_OVF:
                ESP_LOGI("UART:", "hw fifo overflow");
                    uart_flush_input(UART_WIFI_PORT);
                    xQueueReset(uart_wifi_event);
                    break;
                case UART_BUFFER_FULL:
                    uart_flush_input(UART_WIFI_PORT);
                    xQueueReset(uart_wifi_event);
                    break;
                case UART_BREAK:
                    ESP_LOGI("UART:", "uart rx break");
                    break;
                case UART_PARITY_ERR:
                    ESP_LOGI("UART:", "uart parity error");
                    break;
                case UART_FRAME_ERR:
                    ESP_LOGI("UART:", "uart frame error");
                    break;
                default:
                    ESP_LOGI("UART:", "uart wifi_event type: %d", wifi_event.type);
                break;
            }
        free(data);
        data = NULL;
            
        }
    }
    vTaskDelete(NULL);
}

void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (retry_count < MAX_RETRY) {
            ESP_LOGI(WIFI_TAG, "Retry to connect to the AP. Attempt %d.", retry_count);
            esp_wifi_connect();
            retry_count++;
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAILED_BIT);
        }
        ESP_LOGI(WIFI_TAG,"Connect to the AP fail");

    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(WIFI_TAG, "Got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        retry_count = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void Wifi_Task(void *pvParameter)
{
    while(1)
    {
        EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                                WIFI_CONNECTED_BIT | WIFI_FAILED_BIT,
                                                pdTRUE,
                                                pdFALSE,
                                                portMAX_DELAY);

        if( bits & WIFI_CONNECTED_BIT)
        {
            ESP_LOGI(WIFI_TAG, "Wifi connected succesfully.");;
            wifi_state = 1;
        }
        else if( bits & WIFI_FAILED_BIT)
        {
            ESP_LOGI(WIFI_TAG, "Wifi connected failed. Check information again.");
            ESP_LOGI(WIFI_TAG,"SSID: %s; Password: %s", SSID, PASSWORD);
            wifi_state = 0;
            ManualMode = 0;
            uart_write_bytes(UART_DATA_PORT, "No internet, enter Default Automatic mode.\n", 44);
        }
        else
        {
            ESP_LOGE(WIFI_TAG, "Unexpected event.");
        }
    }
}

void http_request(int s, int *r)
{
    while (1)
    {
        sprintf(REQUEST, "GET https://api.thingspeak.com/update?api_key=2J2CAIMN28RURF6E&field1=%d&field2=%d&field3=%d&field4=%d\n\n", temperature, humidity, solidHumidity, water_volume);

        http_write(&s, REQUEST);

        http_set_receive_timeout(&s, 5);

        /* Read HTTP response */
        http_read(s, r, HTTP_RECV);

        close(s);
        break;
    }
}

void http_task(void *pvParameter)
{
    while (1)
    {
        EventBits_t xBit = xEventGroupWaitBits(s_http_event_group,
                                                HTTP_UPDATE_DATA_BIT,
                                                pdTRUE,
                                                pdFALSE,
                                                portMAX_DELAY);
        if( xBit & HTTP_UPDATE_DATA_BIT)
        {
            http_handle();
        }
    }
}

void http_handle()
{
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    }; 
    struct addrinfo *res = NULL;
    struct in_addr *addr = NULL;

    int s = -1;
    int r = -1;

    http_connect(hints, res, addr, &s);

    http_request(s, &r);
}

void UART_slave_mode_send(void *pvParameter)
{
    char uart_mode_req[256];
    while(1)
    {
        EventBits_t u_Bit = xEventGroupWaitBits(s_uart_mode_event_group,
                                                UART_MANUAL_MODE_ON | UART_MANUAL_MODE_OFF,
                                                pdTRUE,
                                                pdFALSE,
                                                portMAX_DELAY);

        if(u_Bit & UART_MANUAL_MODE_ON)
        {
            char slave_mode_set[128];
            // sprintf(slave_mode_set, "Motor PWM: %d , Motor Pump: %d , Lamp: %d .", MotorPWM, MotorPump, Lamp);
            sprintf(slave_mode_set, "Motor PWM: %d .", MotorPWM);
            printf("%s\n", slave_mode_set);
            uart_write_bytes(UART_DATA_PORT, slave_mode_set, strlen(slave_mode_set));
            memset(slave_mode_set, 0, strlen(slave_mode_set));
            vTaskDelay(500 / portTICK_PERIOD_MS);
            sprintf(slave_mode_set, "Motor Pump: %d .", MotorPump);
            printf("%s\n", slave_mode_set);
            uart_write_bytes(UART_DATA_PORT, slave_mode_set, strlen(slave_mode_set));
            memset(slave_mode_set, 0, strlen(slave_mode_set));
            vTaskDelay(500 / portTICK_PERIOD_MS);
            sprintf(slave_mode_set, "Lamp: %d .", Lamp);
            printf("%s\n", slave_mode_set);
            uart_write_bytes(UART_DATA_PORT, slave_mode_set, strlen(slave_mode_set));
            memset(slave_mode_set, 0, strlen(slave_mode_set));
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
        else if (u_Bit & UART_MANUAL_MODE_OFF)
        {
            printf("Manual Mode: 0 .\n");
            MotorPump = 0;
            MotorPWM = 0;
            Lamp = 0;
            char uart_mode_req[256];
            sprintf(uart_mode_req, "Motor PWM: %d , Motor Pump: %d , Lamp: %d ."
                                    , MotorPWM, MotorPump, Lamp);
            printf("%s\n", uart_mode_req);
            uart_write_bytes(UART_DATA_PORT, uart_mode_req, strlen(uart_mode_req));
        }
    }
}

void switch_data_callback(char *data, int len)
{
    ManualMode = atoi(data);
    if(ManualMode)
    {
        printf("Manual Mode ON.\n");
        xEventGroupSetBits(s_uart_mode_event_group, UART_MANUAL_MODE_ON);
    }
    else
    {
        printf("Manual Mode OFF.\n");
        xEventGroupSetBits(s_uart_mode_event_group, UART_MANUAL_MODE_OFF);
    }
}

void motor_mode_callback(char *data, int len)
{
    dataExtract("Motor PWM: ", data, &MotorPWM);
    dataExtract("Motor Pump: ", data, &MotorPump);
    dataExtract("Lamp: ", data, &Lamp);
    printf("Motor PWM: %d, Motor Pump: %d, Lamp: %d.\n",MotorPWM, MotorPump, Lamp);
    xEventGroupSetBits(s_uart_mode_event_group, UART_MANUAL_MODE_ON);
}

void esp32cam_callback(char *data, int len)
{
    dataExtract("{\"Plant Status\": ", data, &plantStatus);
    dataExtract("Plant Status: ", data, &plantStatus);
    printf("Plant status: %d.\n", plantStatus);
    char data_push[100];
    sprintf(data_push, "{\"plantStatus\": %d}", plantStatus);
    get_data_response(data_push, strlen(data_push));
}

void get_data_callback(void)
{
    char data_resp[100];
    sprintf(data_resp, "{\"temperature\": %d, \"humidity\": %d, \"solidHumidity\": %d, \"waterVolume\": %d, \"plantStatus\": %d}",
             temperature, humidity, solidHumidity, water_volume, plantStatus);

    get_data_response(data_resp, strlen(data_resp));
}