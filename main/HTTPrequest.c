#include "esp_http_client.h"
#include "esp_log.h"
#include "../include/HTTPrequest.h"
static const char *TAG = "HTTP_CLIENT";

// ==================== SKICKA DATA ====================
void send_temperature_data(int16_t temp, int16_t hum)
{
    char url[256];

    snprintf(url, sizeof(url),
             "http://%s:%s/Temp/addByGet?temp=%d.%d&hum=%d.%d",
             WEB_HOST, WEB_PORT,
             temp / 10, temp % 10,
             hum / 10, hum % 10);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 5000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK)
    {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "Data skickad! Status = %d", status);
    }
    else
    {
        ESP_LOGE(TAG, "HTTP GET misslyckades: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}

void clear_temperature_data()
{
    char url[256];

    snprintf(url, sizeof(url),
             "http://%s:%s/Temp/Delete",
             WEB_HOST, WEB_PORT);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 5000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_err_t err = esp_http_client_perform(client);
       if (err == ESP_OK)
    {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "Data rensad! Status = %d", status);
    }
    else
    {
        ESP_LOGE(TAG, "HTTP GET misslyckades: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}