/*
    EXERCISE 3: Simple HTTP Protocol with data processing from UART and Sensor
-----------------------------------------------------------------------------------------------------------------------------
    REQUIREMENTS:
        Configure Wifi SSID and Password from UART
        Read Value from a Sensor - HW038
        Using GET POST to post data from Sensor to api.thingspeak.com
-----------------------------------------------------------------------------------------------------------------------------
    DETAILS:
        Use UART-to-USB converter to transfer data (SSID, PASSWORD, CONFIRM) through UART num 1 from computer to ESP32
        The transfer data string needs to contain these format: "ssid ##########" for SSID and "pass ########" for Password
        Transfer data from hw038 to api.thingspeak.com
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <driver/gpio.h>
#include <driver/uart.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_mac.h"
#include "esp_sntp.h"
#include "esp_netif_sntp.h"

#include "lwip/inet.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "sdkconfig.h"

#include "wifi_lib.h"
#include "gpio_init.h"


static char *WIFI_TAG = "Wifi Station Mode";
static char *UART_TAG = "UART Event";

//Define sensor pin
#define HW038_DATA          ADC_CHANNEL_4   //GPIO_NUM_32   //HW308 data pin


//Define UART pin
#define UART_PORT           UART_NUM_1                      //UART port
#define UART_TX             GPIO_NUM_2                      //Data UART Transmit GPIO
#define UART_RX             GPIO_NUM_4                      //Data UART Receive GPIO
#define BUFFER              1024                            //Buffer length unit

static EventGroupHandle_t s_uart_event_group;

#define UART_RECV_BIT       BIT0                            //UART Event Bits
#define UART_PROCESS_BIT    BIT1

static QueueHandle_t uart_event;


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

static char REQUEST[512];                                   //HTTP Request string
static char HTTP_RECV[512];                                 //HTTP Receive string

//RTC 
static time_t now = 0;                                      //Time now
static struct tm timeinfo = {0};                            //Information for getting time

//HTTP data test
#define HW038_RECEIVE_BIT   BIT0                            //HW038 Event Group Bit 

static float hw038 = 0;
static adc_oneshot_unit_handle_t hw038_handle;              //ADC unit handle

void hw038_read(float *HW038);                              //Read HW038 sensor by ADC
void hw038_task(void *pvParameter);                         //Handle HW038 receiving data
float calculate_water_height(float raw_data);               //Calculate water level by input voltage


//Wifi functions
void Wifi_Task(void *pvParameter);                          //Report about Wifi connecting status
void event_handler(void* arg, esp_event_base_t event_base,
                    int32_t event_id, void* event_data);    //Wifi connect events
int Wifi_Info_Finding(char *data, char *ssid, char *pass);  //Examine Data from UART to configure the Wifi
void Wifi_Info_Validication(int validication);              //Verify the Data from UART


//UART functions
void UART_Config(void);                                     //UART Configuring
void UART_Result(void *pvParameter);                        //UART Event Report
void UART_Task(void *pvParameter);                          //UART Receive Data


//HTTP Functions
void http_task(void *pvParameter);                          //HTTP event when an event occurs
void http_handle();                                         //HTTP initiate socket and send request
void http_request(int s, int *r, float HW038);              //Send HTTP request


//SNTP Functions
void initialize_sntp();                                     //RTC initializing
void time_sync_noti_cb(struct timeval *tv);                 //SNTP initialization success callback
void get_time();                                            //Get time from SNTP

void app_main(void)
{
    //Initialize NVS for wifi station mode
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    //Initiate HW038 sensor read
    adc_gpio_init(HW038_DATA, ADC_UNIT_1, &hw038_handle);
    if (hw038_handle == NULL) 
    {
        ESP_LOGE("hw038_read", "hw038_handle is NULL");
    }
    else{
        printf("hw038_handle address: %d\n", (int) &hw038_handle);
    }

    //UART and Wifi Group Event Initializing
    s_uart_event_group = xEventGroupCreate();
    s_wifi_event_group = xEventGroupCreate();
    s_http_event_group = xEventGroupCreate();

    UART_Config();
    
    wifi_init(SSID, PASSWORD, &event_handler, &instance_any_id, &instance_got_ip);

    //SNTP Get Real time
    setenv("TZ", "ICT-7", 1);
    tzset();


    //Task Create
    xTaskCreate(UART_Task, "UART Task", 10*1024, NULL, 14, NULL);
    xTaskCreate(UART_Result, "UART Result", 2*1024, NULL, 13, NULL);
    xTaskCreate(Wifi_Task, "Wifi Task", 2*1024, NULL, 12, NULL);
    
    initialize_sntp();
  
    get_time(&now, &timeinfo);
    xTaskCreate(hw038_task, "Sensor Task", 20*1024, NULL, 10, NULL);
    xTaskCreate(http_task, "HTTP Task", 10*1024, NULL, 11, NULL);
    vTaskDelay(20000 / portTICK_PERIOD_MS);
    while(1)
    {
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        xEventGroupSetBits(s_wifi_event_group, HW038_RECEIVE_BIT);
    }
}

void UART_Config(void)
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .parity = UART_PARITY_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
        .stop_bits = UART_STOP_BITS_1,
    };

    uart_driver_install(UART_PORT, BUFFER*2, BUFFER*2, 20, &uart_event, NULL);
    uart_param_config(UART_PORT, &uart_config);
    uart_set_pin(UART_PORT, UART_TX, UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

void UART_Result(void *pvParameter)
{
    while(1)
    {
        EventBits_t bits = xEventGroupWaitBits(s_uart_event_group,
                                                UART_RECV_BIT | UART_PROCESS_BIT,
                                                pdTRUE,
                                                pdFALSE,
                                                portMAX_DELAY);
        if( bits & UART_RECV_BIT)
        {
            get_time(&now, &timeinfo);
            ESP_LOGI(UART_TAG, "UART received Data.");
        }
        else if (bits & UART_PROCESS_BIT)
        {
            get_time(&now, &timeinfo);
            ESP_LOGI(UART_TAG, "UART Data processed completed.");
        }
    }
}

int Wifi_Info_Finding(char *data, char *ssid, char *pass)
{
    //SSID Finding
    char *pos = strstr((char *) data, "ssid");
    if (pos == NULL){
        if (*ssid == '\0') return NO_SSID;
    }
    else
    {
        memset(ssid, '\0', strlen(ssid));
        int i = 0;
        while (*(pos + 5 + i) != ' ' && *(pos + 5 + i) != '\0')
        {
            *(ssid + i) = *(pos + 5 + i);
            i++;
        }
        *(ssid + i) = '\0';
        if (i == 0) return NO_SSID;
    }


    //Password Finding
    pos = strstr((char *) data, "pass");
    if (pos == NULL){
        if (*pass == '\0') return NO_PASSWORD;
    }
    else
    {
        int i = 0;
        while (*(pos + 5 + i) != ' ' && *(pos + 5 + i) != '\0')
        {
            *(pass + i) = *(pos + 5 + i);
            i++;
        }
        *(pass + i) = '\0';
        if (i == 0) return NO_PASSWORD;
    }

    //Confirm
    pos = strstr((char *) data, "Yes");
    if(pos != NULL) return CONFIRM;

    return VALID;
}

void Wifi_Info_Validication(int validication)
{
    switch (validication)
    {
    case NO_SSID:
        uart_write_bytes(UART_PORT, "No SSID Found, Enter again.\n", 29);
        break;
    
    case NO_PASSWORD:
        uart_write_bytes(UART_PORT, "No Password Found, Enter again.\n", 33);
        break;

    case CONFIRM:
        // wifi_cleanup();
        // wifi_init_sta();
        wifi_state = 0;
        wifi_cleanup(instance_any_id, instance_got_ip);
        wifi_init(SSID, PASSWORD, &event_handler, &instance_any_id, &instance_got_ip);
        break;
    default:
        char buffer[128];
        int len = snprintf(buffer, sizeof(buffer), "SSID: %s, Password: %s.\n", SSID, PASSWORD);
        if (len > 0 && len < sizeof(buffer)) {
            uart_write_bytes(UART_PORT, buffer, len);
        }
        uart_write_bytes(UART_PORT, "Enter \"Yes\" to change Wifi.\n", 29);
        break;
    }
    xEventGroupSetBits(s_uart_event_group, UART_PROCESS_BIT);
}

void UART_Task(void *pvParameter)
{
    uart_event_t event;
    while(1)
    {
        if(xQueueReceive(uart_event, (void *)&event, (TickType_t)portMAX_DELAY))
        {
            get_time(&now, &timeinfo);
            ESP_LOGI(UART_TAG, "UART num %d.", UART_PORT);
            uint8_t *data = (uint8_t *) malloc (BUFFER);
            switch(event.type)
            {
                case UART_DATA:
                    ESP_LOGI("UART:", "[UART DATA]: %d", event.size);
                    uart_read_bytes(UART_PORT, data, event.size, portMAX_DELAY);
                    xEventGroupSetBits(s_uart_event_group, UART_RECV_BIT);
                    Wifi_Info_Validication(Wifi_Info_Finding((char *)data, SSID, PASSWORD));
                    printf("Free memory: %lu\n", esp_get_free_heap_size());
                    break;
                case UART_FIFO_OVF:
                ESP_LOGI("UART:", "hw fifo overflow");
                    uart_flush_input(UART_PORT);
                    xQueueReset(uart_event);
                    break;
                case UART_BUFFER_FULL:
                    uart_flush_input(UART_PORT);
                    xQueueReset(uart_event);
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
                    ESP_LOGI("UART:", "uart event type: %d", event.type);
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
            ESP_LOGI(WIFI_TAG, "Wifi connected succesfully.");
            uart_write_bytes(UART_PORT, "Connected successfully.\n", 25);
            wifi_state = 1;
        }
        else if( bits & WIFI_FAILED_BIT)
        {
            ESP_LOGI(WIFI_TAG, "Wifi connected failed. Check information again.");
            ESP_LOGI(WIFI_TAG,"SSID: %s; Password: %s", SSID, PASSWORD);
            uart_write_bytes(UART_PORT, "Wifi connected failed. Check information again.\n", 49);
            char buffer[128];
            int len = snprintf(buffer, sizeof(buffer), "SSID: %s, Password: %s.\n", SSID, PASSWORD);
            if (len > 0 && len < sizeof(buffer)) uart_write_bytes(UART_PORT, buffer, len);
        }
        else
        {
            ESP_LOGE(WIFI_TAG, "Unexpected event.");
            uart_write_bytes(UART_PORT, "Unexpected event.\n", 19);
        }
    }
}

void time_sync_noti_cb(struct timeval *tv)
{
    printf("Time sync.\n");
}

void initialize_sntp()
{
    ESP_LOGI("SNTP", "Initializing SNTP.");
    esp_sntp_config_t rtc_config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    rtc_config.sync_cb = time_sync_noti_cb;
    esp_netif_sntp_init(&rtc_config);
}

void get_time()
{
    int retry = 0;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < MAX_RETRY)
    {
        ESP_LOGI("SNTP", "Waiting for system time to be set... (%d/%d)", retry, MAX_RETRY);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    time(&now);
    localtime_r(&now, &timeinfo);
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI("SNTP", "Current time: %s", strftime_buf);
}

void hw038_read(float *HW038)
{    
    float old_hw038 = *HW038;
    int hw038_raw;
    ESP_ERROR_CHECK(adc_oneshot_read(hw038_handle, HW038_DATA, &hw038_raw));
    printf("HW038 raw data: %.2f.\n", (float) hw038_raw);
    *HW038 = calculate_water_height((float) hw038_raw);
    printf("HW038: %.2f.\n", *HW038);
}

float calculate_water_height(float raw_data)
{
    if (raw_data < 330) return 0.0;
    if (raw_data > 2000) return 4.5;
    
    float water_height = ((raw_data - 330) / (2000 - 330)) * 4.5;
    
    return water_height;
}

void hw038_task(void *pvParameter)
{
    
    while (1)
    {
        if(wifi_state == 1)
        {
            hw038_read(&hw038);
            xEventGroupSetBits(s_http_event_group, HW038_RECEIVE_BIT);
            vTaskDelay(5000/portTICK_PERIOD_MS);
        }
        vTaskDelay(5000/portTICK_PERIOD_MS);
    }
}

void http_request(int s, int *r, float HW038)
{
    while (1)
    {
        printf("hw038 = %.2f\n", HW038);
        sprintf(REQUEST, "GET https://api.thingspeak.com/update?api_key=2J2CAIMN28RURF6E&field1=%.2f\n\n", HW038);

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
                                                HW038_RECEIVE_BIT,
                                                pdTRUE,
                                                pdFALSE,
                                                portMAX_DELAY);

        if (xBit & HW038_RECEIVE_BIT)
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

    http_request(s, &r, hw038);
}
