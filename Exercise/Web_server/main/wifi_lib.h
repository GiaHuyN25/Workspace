#ifndef WIFI_LIB_H
#define WIFI_LIB_H

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

#define EXAMPLE_ESP_WIFI_SSID      "esp_wifi_set_up"
#define EXAMPLE_ESP_WIFI_PASS      "12345678"
#define EXAMPLE_ESP_WIFI_CHANNEL   1
#define EXAMPLE_MAX_STA_CONN       4

#define MAX_RETRY               10

//Web Server
#define WEB_SERVER  "api.thingspeak.com"
#define WEB_PORT    "80"

//Event Group Bit for Wifi connecting event
#define WIFI_CONNECTED_BIT      BIT0
#define WIFI_FAILED_BIT         BIT1

//Wifi Information Validication type for error handling
typedef enum {
    NOCHANGE, VALID, INVALID, CONFIRM,
} ssid_valid_t;

//Wifi station mode initiation
void wifi_init_sta(char SSID[30], char PASSWORD[30], esp_event_handler_t *wifi_event_handler,
                esp_event_handler_instance_t *wifi_instance_any_id, 
                esp_event_handler_instance_t *wifi_instance_got_ip);

//Wifi cleanup
void wifi_cleanup(esp_event_handler_instance_t wifi_instance_any_id, 
                    esp_event_handler_instance_t wifi_instance_got_ip);

//Connect to web webserver, Initiate address, socket
void http_connect(struct addrinfo http_hints, struct addrinfo *result, 
                    struct in_addr *address, int *http_socket);

//Send socket request
void http_write(int *http_socket, char http_request[512]);

//Read socket response
void http_read(int http_socket, int *http_read, char http_receive[512]);

//Set timeout for receiving response
void http_set_receive_timeout(int *http_socket, int timeout);

#endif