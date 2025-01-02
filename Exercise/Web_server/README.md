| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- |

# Project 1: Read Data sent from Arduino and send to api.thingspeak.com

This exercise is about reading data from Arduino and send to api.thingspeak.com and configurating Wifi through UART

## How to use example

```
UART_DATA_TX pin is GPIO 2
UART_DATA_RX pin is GPIO 4
UART_WIFI_TX pin is GPIO 36
UART_WIFI_RX pin is GPIO 37
```

### Configure the project

```
Change web server and web port in wifi_lib.h 
Change SSID and Password for Wifi in main.c
Change http request link in "void http_request()"
```

### Build and Flash

Build the project and flash it to the board, then run the monitor tool to view the serial output:

Run `idf.py -p PORT flash monitor` to build, flash and monitor the project.

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for all the steps to configure and use the ESP-IDF to build projects.

* [ESP-IDF Getting Started Guide on ESP32](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html)
* [ESP-IDF Getting Started Guide on ESP32-S2](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s2/get-started/index.html)
* [ESP-IDF Getting Started Guide on ESP32-C3](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/get-started/index.html)

## Example Output

```
I (675) wifi:mode : sta (24:58:7c:d7:ba:d4)
I (675) wifi:enable tsf
I (675) WIFI: wifi_init_sta finished.
I (675) HTTP WEBSERVER: Starting server on port: '80'
I (685) HTTP WEBSERVER: Registering URI handlers
I (685) wifi:new:<3,0>, old:<1,0>, ap:<255,255>, sta:<3,0>, prof:1, snd_ch_cfg:0x0
I (695) wifi:state: init -> auth (0xb0)
I (695) UART Event: UART num 2.
I (695) wifi:state: auth -> assoc (0x0)
I (705) UART:: uart rx break
I (705) UART Event: UART num 2.
I (705) wifi:I (715) UART: [UART DATA]: 1
state: assoc -> run (0x10)I (715) UART:


I (705) main_task: Returned from app_main()
I (735) wifi:connected with Phong301_an_trai, aid = 4, channel 3, BW20, bssid = 90:6a:94:2c:ba:2e
I (735) wifi:security: WPA2-PSK, phy: bgn, rssi: -29
I (745) wifi:pm start, type: 1

I (745) wifi:dp: 1, bi: 102400, li: 3, scale listen interval from 307200 us to 307200 us
I (755) wifi:set rx beacon pti, rx_bcn_pti: 0, bcn_timeout: 25000, mt_pti: 0, mt_time: 10000
I (755) wifi:AP's beacon interval = 102400 us, DTIM period = 1
I (1765) esp_netif_handlers: sta ip: 192.168.10.17, mask: 255.255.255.0, gw: 192.168.10.1
I (1765) Wifi Station Mode: Got ip:192.168.10.17
I (1765) Wifi Station Mode: Wifi connected succesfully.
```


## Troubleshooting

For any technical queries, please open an [issue](https://github.com/espressif/esp-idf/issues) on GitHub. We will get back to you soon.
