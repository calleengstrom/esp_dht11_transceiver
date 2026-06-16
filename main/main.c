
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "../include/wifi_connect.h"
#include "../include/dht.h"

void app_main(void)
{
    dht_sensor_type_t dht_sensor = DHT_TYPE_DHT11;
    gpio_num_t gpio_pin = GPIO_NUM_32;
    int16_t hum = 0;
    int16_t temp = 0;

    // Test GPIO32 nivå före DHT-läsning
    gpio_set_direction(gpio_pin, GPIO_MODE_INPUT);
    vTaskDelay(pdMS_TO_TICKS(100));
    int initial_level = gpio_get_level(gpio_pin);
    ESP_LOGI("main_task", "GPIO32 initial level: %d (should be 1 if pulled up)", initial_level);

    // DHT11 behöver ofta upp till 1-2 sekunder efter ström på för att bli redo.
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGI("main_task", "Reading DHT11 on GPIO%d", gpio_pin);
    esp_err_t res = dht_read_data(dht_sensor, gpio_pin, &hum, &temp);
    if (res == ESP_OK) {
        ESP_LOGI("main_task", "Humidity=%d.%d%%, Temperature=%d.%d°C",
                 hum / 10, hum % 10, temp / 10, temp % 10);
    }

    // wifi_connect();
}
