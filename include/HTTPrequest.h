#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define WEB_HOST "192.168.0.192"
#define WEB_PORT "8080"
#define WEB_PATH "/"
void send_temperature_data(int16_t temp, int16_t hum);
void clear_temperature_data(void);
#endif