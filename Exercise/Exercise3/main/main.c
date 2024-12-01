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

//Config security modes
#if CONFIG_ESP_WPA3_SAE_PWE_HUNT_AND_PECK
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
#define H2E_IDENTIFIER ""
#elif CONFIG_ESP_WPA3_SAE_PWE_HASH_TO_ELEMENT
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HASH_TO_ELEMENT
#define H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#elif CONFIG_ESP_WPA3_SAE_PWE_BOTH
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#endif

#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

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
#define MAX_RETRY      10

static EventGroupHandle_t s_wifi_event_group;

#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAILED_BIT     BIT1

static char SSID[30] = "Phong301_an_trai";
static char PASSWORD[30] = "12344321";

static esp_event_handler_instance_t instance_any_id;
static esp_event_handler_instance_t instance_got_ip;

typedef enum {
    VALID, NO_SSID, NO_PASSWORD, CONFIRM,
} ssid_valid_t;

static int retry_count = 0;
static int retry = 0;

//HTTP Protocol
static char REQUEST[512];
static char SUBREQUEST[128];
static char HTTP_RECV[512];

#define WEB_SERVER  "api.thingspeak.com"
#define WEB_PORT    "80"

const struct addr_info hints = {
    .ai_family = AF_INET,
    .ai_socktype = SOCK_STREAM,
};

static struct addr_info *res;
static struct in_addr *addr;
static int s, r;


//RTC 
static time_t now = 0;
static struct tm timeinfo = {0};


//Wifi functions
void Wifi_Task(void *pvParameter);                          //Report about Wifi connecting status
void event_handler(void* arg, esp_event_base_t event_base,
                    int32_t event_id, void* event_data);    //Wifi connect events
void wifi_init_sta(void);                                   //Configuring and Connecting Wifi
int Wifi_Info_Finding(char *data, char *ssid, char *pass);  //Examine Data from UART to configure the Wifi
void Wifi_Info_Validication(int validication);              //Verify the Data from UART
void wifi_cleanup();                                        //Cleanup Wifi Config before start new one


//UART functions
void UART_Config(void);                                     //UART Configuring
void UART_Result(void *pvParameter);                        //UART Event Report
void UART_Task(void *pvParameter);                          //UART Receive Data


//HTTP Functions
// static void http_task(void *pvParameter);


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

    UART_Config();
    wifi_init_sta();


    //HTTP Initializing


    //SNTP Get Real time
    setenv("TZ", "ICT-7", 1);
    tzset();

    initialize_sntp();

    

    //Task Create
    xTaskCreate(UART_Task, "UART Task", 10*1024, NULL, 11, NULL);
    xTaskCreate(UART_Result, "UART Result", 2*1024, NULL, 10, NULL);
    xTaskCreate(Wifi_Task, "Wifi Task", 2*1024, NULL, 9, NULL);
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
            get_time();
            ESP_LOGI(UART_TAG, "UART received Data.");
        }
        else if (bits & UART_PROCESS_BIT)
        {
            get_time();
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
        retry_count = 0;
        wifi_cleanup();
        wifi_init_sta();
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
            get_time();
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

void wifi_init_sta(void)
{

    ESP_ERROR_CHECK(esp_netif_init());
    
    esp_err_t err = esp_event_loop_create_default();
    if (err == ESP_ERR_INVALID_STATE) {
        ESP_LOGW("WIFI_INIT", "Default event loop already created, skipping.");
    } else {
        ESP_ERROR_CHECK(err);
    }

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    //Init Wifi properties
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",

            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e = ESP_WIFI_SAE_MODE,
            .sae_h2e_identifier = H2E_IDENTIFIER,
        }
    };

    strncpy((char *)wifi_config.sta.ssid, SSID, sizeof(wifi_config.sta.ssid));              //Wifi ssid and password
    strncpy((char *)wifi_config.sta.password, PASSWORD, sizeof(wifi_config.sta.password));  //that can be assigned by
                                                                                            //string

    wifi_config.sta.ssid[sizeof(wifi_config.sta.ssid) - 1] = '\0';                          //Make sure the string 
    wifi_config.sta.password[sizeof(wifi_config.sta.password) - 1] = '\0';                  //ends by "\0"

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_bandwidth(WIFI_IF_STA, WIFI_BW_HT20));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(WIFI_TAG, "wifi_init_sta finished.");

}

void wifi_cleanup() {
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    esp_netif_destroy(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"));
    ESP_ERROR_CHECK(esp_wifi_deinit());
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
    ESP_LOGI("SNTP", "Notification of a time synchronization event.");
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
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < MAX_RETRY)
    {
        ESP_LOGI("SNTP:", "Waiting for system time to be set... (%d/%d)", retry, MAX_RETRY);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    time(&now);
    localtime_r(&now, &timeinfo);
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI("SNTP", "Current time: %s", strftime_buf);
}

static void http_task(void *pvParameter)
{
    while (1)
    {
        int err = getaddrinfo(WEB_SERVER, WEB_PORT, &hints, &res);

        if (err == 0 || err == NULL)
        {
            
            ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        addr = &((struct sockaddr_in *)res->ai_addr) -> sin_addr;
        ESP_LOGI(HTTP_TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

        s = socket(res->ai_family, res->ai_socktype, 0); //Create socket
        if(s < 0) {
            ESP_LOGE(HTTP_TAG, "... Failed to allocate socket.");
            freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(HTTP_TAG, "... allocated socket");

        if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            ESP_LOGE(HTTP_TAG, "... socket connect failed errno=%d", errno);
            close(s);
            freeaddrinfo(res);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }

        ESP_LOGI(HTTP_TAG, "... connected");
        freeaddrinfo(res);

        sprintf(REQUEST, "GET https://api.thingspeak.com/channels/2724105/feeds.json?api_key=AAW8B9JI1WFH4UER&results=2\n\n");

        if (write(s, REQUEST, strlen(REQUEST)) < 0) {
            ESP_LOGE(HTTP_TAG, "... socket send failed errno=%d", errno);
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(HTTP_TAG, "... socket send success");

        struct timeval receiving_timeout;
        receiving_timeout.tv_sec = 5;
        receiving_timeout.tv_usec = 0;
        if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
                sizeof(receiving_timeout)) < 0) {
            ESP_LOGE(HTTP_TAG, "... failed to set socket receiving timeout");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(HTTP_TAG, "... set socket receiving timeout success");

        /* Read HTTP response */
        do {
            bzero(SUBREQUEST, sizeof(SUBREQUEST));
            r = read(s, SUBREQUEST, sizeof(SUBREQUEST)-1);
            for(int i = 0; i < r; i++) {
                putchar(SUBREQUEST[i]);
            }
        } while(r > 0);

        ESP_LOGI(HTTP_TAG, "... done reading from socket. Last read return=%d errno=%d.", r, errno);
        close(s);
        for(int countdown = 10; countdown >= 0; countdown--) {
            ESP_LOGI(HTTP_TAG, "%d... ", countdown);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(HTTP_TAG, "Starting again!");

    }
}