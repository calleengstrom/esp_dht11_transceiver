
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

#include "../include/HTTPrequest.h"



void wifi_connet_send_temp(int16_t hum, int16_t temp)
{
    wifi_connect();
    vTaskDelay(pdMS_TO_TICKS(500));
    clear_temperature_data();
    send_temperature_data(temp, hum);
    esp_wifi_stop();
    vTaskDelay(pdMS_TO_TICKS(500));

}

void dht_task(void *args)
{

    dht_sensor_type_t dht_sensor = DHT_TYPE_DHT11;
    gpio_num_t gpio_pin = GPIO_NUM_4;
    int16_t hum = 0;
    int16_t temp = 0;
    while (1)
    {

        vTaskDelay(pdMS_TO_TICKS(5000));

        esp_err_t res = dht_read_data(dht_sensor, gpio_pin, &hum, &temp);

        if (res == ESP_OK)
        {
            ESP_LOGI("main_task", "Humidity=%d.%d%%, Temperature=%d.%d°C",
                     hum / 10, hum % 10, temp / 10, temp % 10);
            
           wifi_connet_send_temp(hum,temp);
        }
    }
}

void app_main(void)
{

    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_INPUT);
    vTaskDelay(pdMS_TO_TICKS(100));
    int initial_level = gpio_get_level(GPIO_NUM_4);
    ESP_LOGI("main_task", "GPIO32 initial level: %d (should be 1 if pulled up)", initial_level);

    xTaskCreatePinnedToCore(dht_task, "dht_task", 8192, NULL, 22, NULL, 0);
}
