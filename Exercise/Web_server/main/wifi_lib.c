#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_mac.h"
#include "esp_sntp.h"
#include "esp_netif_sntp.h"
#include "esp_attr.h"

#include "lwip/inet.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "wifi_lib.h"

static const char *TAG = "wifi softAP";

void wifi_init_sta(char SSID[30], char PASSWORD[30], esp_event_handler_t *wifi_event_handler,
                esp_event_handler_instance_t *wifi_instance_any_id, 
                esp_event_handler_instance_t *wifi_instance_got_ip)
{
    esp_netif_destroy(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"));

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
                                                        wifi_event_handler,
                                                        NULL,
                                                        wifi_instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        wifi_event_handler,
                                                        NULL,
                                                        wifi_instance_got_ip));

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

    ESP_LOGI("WIFI", "wifi_init_sta finished.");
}

void wifi_cleanup(esp_event_handler_instance_t wifi_instance_any_id, 
                    esp_event_handler_instance_t wifi_instance_got_ip) {
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_instance_got_ip));
    esp_netif_destroy(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"));
    ESP_ERROR_CHECK(esp_wifi_deinit());
}

void http_connect(struct addrinfo http_hints, struct addrinfo *result, 
                    struct in_addr *address, int *http_socket)
{
    while(1) {
        int err = getaddrinfo(WEB_SERVER, WEB_PORT, &http_hints, &result);

        if(err != 0 || result == NULL) {
            ESP_LOGE("HTTP", "DNS lookup failed err=%d result=%p", err, result);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        address = &((struct sockaddr_in *)result->ai_addr) -> sin_addr;
        ESP_LOGI("HTTP", "DNS lookup succeeded. IP=%s", inet_ntoa(*address));

        *http_socket = socket(result->ai_family, result->ai_socktype, 0); //Create socket
        if(*http_socket < 0) {
            ESP_LOGE("HTTP", "... Failed to allocate socket.");
            freeaddrinfo(result);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI("HTTP", "... allocated socket");

        if(connect(*http_socket, result->ai_addr, result->ai_addrlen) != 0) {
            ESP_LOGE("HTTP", "... socket connect failed errno=%d", errno);
            close(*http_socket);
            freeaddrinfo(result);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }

        ESP_LOGI("HTTP", "... connected");
        freeaddrinfo(result);
        break;
    }
}

void http_write(int *http_socket, char http_request[512])
{
    while(1)
    {
        if (write(*http_socket, http_request, strlen(http_request)) < 0) {
            ESP_LOGE("HTTP", "... socket send failed errno=%d", errno);
            close(*http_socket);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI("HTTP", "... socket send success");
        break;
    }
}

void http_read(int http_socket, int *http_read, char http_receive[512])
{
    while(1)
    {
        do {
            bzero(http_receive, sizeof(http_receive));
            *http_read = read(http_socket, http_receive, sizeof(http_receive)-1);
            for(int i = 0; i < *http_read; i++) {
                putchar(http_receive[i]);
            }
        } while(*http_read > 0);
        printf("\n");
        ESP_LOGI("HTTP", "... done reading from socket. Last read return=%d errno=%d.", *http_read, errno);
        break;

    }
}

void http_set_receive_timeout(int *http_socket, int timeout)
{
    struct timeval receiving_timeout;   
    receiving_timeout.tv_sec = timeout;
    receiving_timeout.tv_usec = 0;
    while(1)
    {
        if (setsockopt(*http_socket, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
            sizeof(receiving_timeout)) < 0) {
            ESP_LOGE("HTTP", "... failed to set socket receiving timeout");
            close(*http_socket);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI("HTTP", "... set socket receiving timeout success");
        break;
    }
}