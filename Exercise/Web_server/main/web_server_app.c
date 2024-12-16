/* Simple HTTP Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "esp_netif.h"
#include "esp_tls_crypto.h"
#include <esp_http_server.h>
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_tls.h"
#include "esp_check.h"

#include "web_server_app.h"

static const char *TAG = "HTTP WEBSERVER";
static httpd_handle_t server;
static httpd_req_t *REQ;

extern const uint8_t easter_egg_start[] asm("_binary_vo_toi_gif_start");
extern const uint8_t easter_egg_end[] asm("_binary_vo_toi_gif_end");
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");
// extern const uint8_t apindex_html_start[] asm("_binary_apindex_html_start");
// extern const uint8_t apindex_html_end[] asm("_binary_apindex_html_end");

static http_post_callback_t http_post_switch_callback = NULL;
static http_post_callback_t http_post_motor_mode_callback = NULL;
static http_post_callback_t http_post_esp32cam_callback = NULL;
static http_get_callback_t  http_get_data_callback = NULL;

static esp_err_t post_data_handler(httpd_req_t *req)
{
    char buf[100];

    httpd_req_recv(req, buf, req -> content_len);
    printf("DATA: %s\n", buf);

    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t post_data = {
    .uri       = "/data",
    .method    = HTTP_POST,
    .handler   = post_data_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};

static esp_err_t post_switch1_handler(httpd_req_t *req)
{
    char buf[100];

    httpd_req_recv(req, buf, req -> content_len);
    http_post_switch_callback(buf, req -> content_len);

    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t post_switch1_data = {
    .uri       = "/switch1",
    .method    = HTTP_POST,
    .handler   = post_switch1_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};

static esp_err_t motor_mode_handler(httpd_req_t *req)
{
    char buf[100];
    httpd_req_recv(req, buf, req -> content_len);
    printf("%s\n", buf);
    http_post_motor_mode_callback(buf, req -> content_len);
    printf("Motor Mode Triggered.\n");

    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t post_motor_mode_data = {
    .uri       = "/motorMode",
    .method    = HTTP_POST,
    .handler   = motor_mode_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};

static esp_err_t esp32cam_handler(httpd_req_t *req)
{
    char buf[100];

    httpd_req_recv(req, buf, req -> content_len);
    printf("%s\n", buf);
    http_post_esp32cam_callback(buf, req -> content_len);
    
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t post_esp32cam_data = {
    .uri       = "/camCheck",
    .method    = HTTP_POST,
    .handler   = esp32cam_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};

static esp_err_t interface_handler(httpd_req_t *req)
{
    // httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char*) index_html_start, index_html_end - index_html_start);
    return ESP_OK;
}

static const httpd_uri_t interface = {
    .uri       = "/interface",
    .method    = HTTP_GET,
    .handler   = interface_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};

void get_data_response(char *data, int len)
{
    httpd_resp_send(REQ, data, len);
}

static esp_err_t http_get_data_handler(httpd_req_t *req)
{
    REQ = req;
    http_get_data_callback();
    
    return ESP_OK;
}

static const httpd_uri_t http_get_data = {
    .uri       = "/getData",
    .method    = HTTP_GET,
    .handler   = http_get_data_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};


static esp_err_t easter_egg_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "image/gif");
    httpd_resp_send(req, (const char*) easter_egg_start, easter_egg_end - easter_egg_start);
    return ESP_OK;
}

static const httpd_uri_t http_easter_egg = {
    .uri       = "/voToi",
    .method    = HTTP_GET,
    .handler   = easter_egg_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};

esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    if (strcmp("/dht11", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/hello URI is not available");
        /* Return ESP_OK to keep underlying socket open */
        return ESP_OK;
    }
    /* For any other URI send 404 and close socket */
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
    return ESP_FAIL;
}


void start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();



    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &interface);
        httpd_register_uri_handler(server, &http_get_data);
        httpd_register_uri_handler(server, &http_easter_egg);
        httpd_register_uri_handler(server, &post_data);
        httpd_register_uri_handler(server, &post_switch1_data);
        httpd_register_uri_handler(server, &post_motor_mode_data);
        httpd_register_uri_handler(server, &post_esp32cam_data);
        httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, &http_404_error_handler);
    }
    else
    {
        ESP_LOGE("WEB_SERVER", "Cannot start HTTP server.");
    }
}


void stop_webserver()
{
    // Stop the httpd server
    httpd_stop(server);
}

void http_set_callback_switch(void *cb)
{
    http_post_switch_callback = cb;
}

void http_set_callback_motor(void *cb)
{
    http_post_motor_mode_callback = cb;
}

void http_set_callback_esp32cam(void *cb)
{
    http_post_esp32cam_callback = cb;
}

void http_set_callback_get_data(void *cb)
{
    http_get_data_callback = cb;
}