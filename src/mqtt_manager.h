#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>

// MQTT Broker configuration
#define MQTT_BROKER_URI    "mqtt://alderaan.software-engineering.ie:1883"

// QoS levels
#define MQTT_QOS_ALERTS     1   // At-least-once for critical alerts
#define MQTT_QOS_TELEMETRY  0   // At-most-once for bulk telemetry

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
