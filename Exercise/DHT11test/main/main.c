#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"

#define DHT11_PIN GPIO_NUM_4 // Chọn chân GPIO kết nối với DHT11

// Các hàm delay chính xác
void delay_us(uint32_t us) {
    ets_delay_us(us);
}

// Hàm khởi tạo giao tiếp với DHT11
void dht11_start() {
    gpio_set_direction(DHT11_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT11_PIN, 0);
    delay_us(20000); // Kéo xuống mức thấp trong 20ms
    gpio_set_level(DHT11_PIN, 1);
    delay_us(40);    // Kéo lên mức cao 40us
    gpio_set_direction(DHT11_PIN, GPIO_MODE_INPUT);
}

// Đọc một bit từ DHT11
uint8_t dht11_read_bit() {
     
    delay_us(40);
    
    uint8_t bit = 0;
    if(gpio_get_level(DHT11_PIN) == 1)
        bit = 1;
        
    while(gpio_get_level(DHT11_PIN) == 1);
    return bit;
}

// Đọc một byte từ DHT11
uint8_t dht11_read_byte() {
    uint8_t data = 0;
    for(int i = 0; i < 8; i++) {
        data = (data << 1) | dht11_read_bit();
    }
    return data;
}

// Đọc dữ liệu từ DHT11
int dht11_read(float *humidity, float *temperature) {
    uint8_t data[5] = {0};
    
    dht11_start();
    
    // Chờ phản hồi từ DHT11
    if(gpio_get_level(DHT11_PIN) == 0) {
        // Đọc 5 byte dữ liệu
        for(int i = 0; i < 5; i++) {
            data[i] = dht11_read_byte();
        }
        
        // Kiểm tra checksum
        uint8_t checksum = data[0] + data[1] + data[2] + data[3];
        if(checksum == data[4]) {
            *humidity = data[0] + data[1] * 0.1;
            *temperature = data[2] + data[3] * 0.1;
            return 0;
        }
    }
    
    return -1;
}

void app_main() {
    // Cấu hình GPIO cho DHT11
    gpio_reset_pin(DHT11_PIN);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    
    while(1) {
        float humidity, temperature;
        
        if(dht11_read(&humidity, &temperature) == 0) {
            printf("Nhiệt độ: %.1f°C, Độ ẩm: %.1f%%\n", temperature, humidity);
        } else {
            printf("Lỗi đọc cảm biến DHT11\n");
        }
        
        // Chờ 2 giây trước khi đọc lại
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}