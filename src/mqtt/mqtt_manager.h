#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief Initialize MQTT client
 * @return ESP_OK on success
 */
esp_err_t mqtt_manager_init(void);

/**
 * @brief Start the MQTT client connection
 * @return ESP_OK on success
 */
esp_err_t mqtt_manager_start(void);

/**
 * @brief Check if MQTT is currently connected
 * @return true if connected to broker
 */
bool mqtt_manager_is_connected(void);

/**
 * @brief The mqtt_task FreeRTOS task function
 * @param pvParameters Task parameters (unused)
 */
void mqtt_task(void *pvParameters);

#endif // MQTT_MANAGER_H
