/*
    EXERCISE 3: Simple HTTP Protocol with data processing from UART and Sensor
-----------------------------------------------------------------------------------------------------------------------------
    REQUIREMENTS:
        Configure Wifi SSID and Password from UART
        Read Value from a Sensor
        Using GET POST to post data from Sensor to api.thingspeak.com
-----------------------------------------------------------------------------------------------------------------------------
    DETAILS:
        Use UART-to-USB converter to transfer data (SSID, PASSWORD, CONFIRM) through UART num 1 from computer to ESP32
        The transfer data string needs to contain these format: "ssid ##########" for SSID and "pass ########" for Password
        

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


static char *WIFI_TAG = "Wifi Station Mode";
static char *UART_TAG = "UART Event";
static char *HTTP_TAG = "HTTP Event";


//Define UART pin
#define UART_PORT           UART_NUM_1
#define UART_TX             GPIO_NUM_2
#define UART_RX             GPIO_NUM_4
#define BUFFER              1024

static EventGroupHandle_t s_uart_event_group;

#define UART_RECV_BIT       BIT0    //UART Event Bits
#define UART_PROCESS_BIT    BIT1

static QueueHandle_t uart_event;


//Wifi
static EventGroupHandle_t s_wifi_event_group;

static char SSID[30] = "Phong301_an_trai";
static char PASSWORD[30] = "12344321";

static esp_event_handler_instance_t instance_any_id;
static esp_event_handler_instance_t instance_got_ip;

    static int retry_count = 0;

//HTTP Protocol
static EventGroupHandle_t s_http_event_group;

#define HW038_RECEIVE_BIT   BIT0

static char REQUEST[512];
static char SUBREQUEST[128];
static char HTTP_RECV[512];

typedef enum http_data
{
    HW038_event = 1,
    DHT11_temp_event = 2,
    DHT11_hum_event = 3,
}http_data_t;

//HTTP data test
static int hw038 = 0;

void test_task(void *pvParameter)
{
    while(1)
    {
        hw038 =  rand() % 101;
        printf("%d\n", hw038);
        xEventGroupSetBits(s_http_event_group, HW038_RECEIVE_BIT);
        vTaskDelay(20000/portTICK_PERIOD_MS);
    }
}


//RTC 
static time_t now = 0;
static struct tm timeinfo = {0};

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
void http_task(void *pvParameter);
void http_handle();
void http_request(int s, int *r, int HW038);


//SNTP Functions
void initialize_sntp();                                     //RTC initializing
void time_sync_noti_cb(struct timeval *tv);                 //SNTP initialization success callback
void get_time();

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);


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

    xTaskCreate(test_task, "Test Task", 2*1024, NULL, 11, NULL);
    xTaskCreate(http_task, "HTTP Task", 10*1024, NULL, 10, NULL);

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
    printf("Time synced: %lld\n", tv->tv_sec);
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

void http_request(int s, int *r, int HW038)
{
    while (1)
    {
        printf("hw038 = %d\n", HW038);
        sprintf(REQUEST, "GET https://api.thingspeak.com/update?api_key=2J2CAIMN28RURF6E&field1=%d\n\n", HW038);

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