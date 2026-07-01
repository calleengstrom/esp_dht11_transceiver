#include "../include/dht.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "dht";

#define DHT_TIMER_INTERVAL_US     2
#define DHT_DATA_BITS             40
#define DHT_DATA_BYTES            (DHT_DATA_BITS / 8)

static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

#define ENTER_CRITICAL()   portENTER_CRITICAL(&mux)
#define EXIT_CRITICAL()    portEXIT_CRITICAL(&mux)

#define CHECK_ARG(x) do { if (!(x)) return ESP_ERR_INVALID_ARG; } while(0)

static esp_err_t dht_await_pin_state(gpio_num_t pin, uint32_t timeout_us,
                                     uint32_t expected_state, uint32_t *duration)
{
    uint64_t start = esp_timer_get_time();
    uint64_t now;

    gpio_set_direction(pin, GPIO_MODE_INPUT);

    while (true) {
        now = esp_timer_get_time();
        if (now - start >= timeout_us) {
            if (duration) *duration = (uint32_t)(now - start);
            return ESP_ERR_TIMEOUT;
        }

        if (gpio_get_level(pin) == expected_state) {
            if (duration) *duration = (uint32_t)(now - start);
            return ESP_OK;
        }
    }
}

static esp_err_t dht_fetch_data(dht_sensor_type_t sensor_type, gpio_num_t pin, uint8_t data[DHT_DATA_BYTES])
{
    uint8_t byte_idx = 0, bit_idx = 0;

    // Phase A: Start signal with open-drain release
    gpio_set_direction(pin, GPIO_MODE_OUTPUT_OD);
    gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);
    gpio_set_level(pin, 0);
    esp_rom_delay_us(sensor_type == DHT_TYPE_SI7021 ? 500 : 25000);

    gpio_set_level(pin, 1);
    esp_rom_delay_us(40);

    gpio_set_direction(pin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);

    // Phase B, C, D
    if (dht_await_pin_state(pin, 20000, 0, NULL) != ESP_OK) {
        return ESP_ERR_TIMEOUT;
    }
    if (dht_await_pin_state(pin, 20000, 1, NULL) != ESP_OK) {
        return ESP_ERR_TIMEOUT;
    }
    if (dht_await_pin_state(pin, 20000, 0, NULL) != ESP_OK) {
        return ESP_ERR_TIMEOUT;
    }

    // Läs 40 bits som i Arduino-koden
    for (int i = 0; i < DHT_DATA_BITS; i++) {
        if (dht_await_pin_state(pin, 10000, 1, NULL) != ESP_OK) {
            return ESP_ERR_TIMEOUT;
        }

        esp_rom_delay_us(30);

        byte_idx = i / 8;
        bit_idx = i % 8;
        if (bit_idx == 0) data[byte_idx] = 0;

        if (gpio_get_level(pin) == 1) {
            data[byte_idx] |= (1 << (7 - bit_idx));
            if (dht_await_pin_state(pin, 10000, 0, NULL) != ESP_OK) {
                return ESP_ERR_TIMEOUT;
            }
        }
    }

    return ESP_OK;
}

esp_err_t dht_read_data(dht_sensor_type_t sensor_type, gpio_num_t pin,
                        int16_t *humidity, int16_t *temperature)
{
    CHECK_ARG(humidity || temperature);

    uint8_t data[DHT_DATA_BYTES] = {0};
    esp_err_t ret;

    ENTER_CRITICAL();
    ret = dht_fetch_data(sensor_type, pin, data);
    EXIT_CRITICAL();

    // Återställ pinne
    gpio_set_direction(pin, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(pin, 1);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Timeout waiting for DHT sensor response");
        return ret;
    }

    // Checksum
    if (data[4] != ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
        ESP_LOGE(TAG, "Checksum error from DHT sensor: %02x %02x %02x %02x %02x",
                 data[0], data[1], data[2], data[3], data[4]);
        return ESP_ERR_INVALID_CRC;
    }

    if (humidity) {
        if (sensor_type == DHT_TYPE_DHT11)
            *humidity = data[0] * 10;
        else
            *humidity = (data[0] << 8) | data[1];
    }

    if (temperature) {
        if (sensor_type == DHT_TYPE_DHT11)
            *temperature = data[2] * 10;
        else {
            uint16_t raw = (data[2] << 8) | data[3];
            *temperature = (data[2] & 0x80) ? -(raw & 0x7FFF) : raw;
        }
    }

    return ESP_OK;
}

esp_err_t dht_read_float_data(dht_sensor_type_t sensor_type, gpio_num_t pin,
                              float *humidity, float *temperature)
{
    CHECK_ARG(humidity || temperature);

    int16_t i_hum = 0, i_temp = 0;
    esp_err_t res = dht_read_data(sensor_type, pin,
                                  humidity ? &i_hum : NULL,
                                  temperature ? &i_temp : NULL);

    if (res == ESP_OK) {
        if (humidity)   *humidity   = i_hum / 10.0f;
        if (temperature)*temperature = i_temp / 10.0f;
    }
    return res;
}