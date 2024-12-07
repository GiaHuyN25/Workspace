| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- |

# Exercise 3: Read Data from HW038 and send to api.thingspeak.com

This exercise is about reading HW038 data and send to api.thingspeak.com and configurating Wifi through UART

## How to use example

```
HW038 data pin connect to GPIO 32
UART TX pin is GPIO 2
UART RX pin is GPIO 4
```

### Configure the project

```
Change web server and web port in wifi_lib.h 
Change SSID and Password for Wifi in main.c
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
I (559) main_task: Calling app_main()
Initiated ADC.
hw038_handle address: 1073432220
I (589) uart: queue free spaces: 20
I (609) wifi:wifi driver task: 3ffc2308, prio:23, stack:6656, core=0     
I (629) wifi:wifi firmware version: ccaebfa
I (629) wifi:wifi certification version: v7.0
I (629) wifi:config NVS flash: enabled
I (629) wifi:config nano formating: disabled
I (629) wifi:Init data frame dynamic rx buffer num: 32
I (639) wifi:Init static rx mgmt buffer num: 5
I (639) wifi:Init management short buffer num: 32
I (649) wifi:Init dynamic tx buffer num: 32
I (649) wifi:Init static rx buffer size: 1600
I (649) wifi:Init static rx buffer num: 10
I (659) wifi:Init dynamic rx buffer num: 32
I (659) wifi_init: rx ba win: 6
I (669) wifi_init: accept mbox: 6
I (669) wifi_init: tcpip mbox: 32
I (669) wifi_init: udp mbox: 6
I (679) wifi_init: tcp mbox: 6
I (679) wifi_init: tcp tx win: 5760
I (689) wifi_init: tcp rx win: 5760
I (689) wifi_init: tcp mss: 1440
I (689) wifi_init: WiFi IRAM OP enabled
I (699) wifi_init: WiFi RX IRAM OP enabled
I (709) phy_init: phy_version 4830,54550f7,Jun 20 2024,14:22:08
W (799) phy_init: saving new calibration data because of checksum failure, mode(0)
I (859) wifi:mode : sta (a8:42:e3:a8:66:b8)
I (859) wifi:enable tsf
I (859) WIFI: wifi_init_sta finished.
I (859) SNTP: Initializing SNTP.
I (869) SNTP: Waiting for system time to be set... (1/10)
I (869) wifi:new:<3,0>, old:<1,0>, ap:<255,255>, sta:<3,0>, prof:1, snd_ch_cfg:0x0
I (869) wifi:state: init -> auth (0xb0)
I (879) wifi:state: auth -> assoc (0x0)
I (889) wifi:state: assoc -> run (0x10)
I (899) wifi:connected with Phong301_an_trai, aid = 5, channel 3, BW20, bssid = 90:6a:94:2c:ba:2e
I (899) wifi:security: WPA2-PSK, phy: bgn, rssi: -40
I (909) wifi:pm start, type: 1
```

```
I (909) wifi:dp: 1, bi: 102400, li: 3, scale listen interval from 307200 us to 307200 us
I (919) wifi:AP's beacon interval = 102400 us, DTIM period = 1
I (1919) esp_netif_handlers: sta ip: 192.168.10.13, mask: 255.255.255.0, gw: 192.168.10.1
I (1919) Wifi Station Mode: Got ip:192.168.10.13
I (1919) Wifi Station Mode: Wifi connected succesfully.
I (2869) SNTP: Waiting for system time to be set... (2/10)
I (3609) wifi:<ba-add>idx:0 (ifx:0, 90:6a:94:2c:ba:2e), tid:0, ssn:4, winSize:64
I (4869) SNTP: Waiting for system time to be set... (3/10)
I (6869) SNTP: Waiting for system time to be set... (4/10)
I (8869) SNTP: Waiting for system time to be set... (5/10)
I (10869) SNTP: Waiting for system time to be set... (6/10)
I (12869) SNTP: Waiting for system time to be set... (7/10)
I (14869) SNTP: Waiting for system time to be set... (8/10)
I (16869) SNTP: Waiting for system time to be set... (9/10)
I (18869) SNTP: Current time: Thu Jan  1 07:00:21 1970
HW038 raw data: 0.00.
HW038: 0.00.
I (19299) HTTP: DNS lookup succeeded. IP=54.81.142.230
I (19299) HTTP: ... allocated socket
I (19559) wifi:<ba-add>idx:1 (ifx:0, 90:6a:94:2c:ba:2e), tid:1, ssn:0, winSize:64
I (19569) HTTP: ... connected
hw038 = 0.00
I (19579) HTTP: ... socket send success
I (19579) HTTP: ... set socket receiving timeout success
177
I (19899) HTTP: ... done reading from socket. Last read return=0 errno=128.
```


## Troubleshooting

For any technical queries, please open an [issue](https://github.com/espressif/esp-idf/issues) on GitHub. We will get back to you soon.
