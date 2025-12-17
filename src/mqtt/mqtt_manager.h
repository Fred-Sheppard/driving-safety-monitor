#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>

esp_err_t mqtt_manager_init(void);
esp_err_t mqtt_manager_start(void);
bool mqtt_manager_is_connected(void);
void mqtt_task(void *pvParameters);

#endif // MQTT_MANAGER_H
