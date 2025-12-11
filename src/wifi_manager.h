#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

// WiFi credentials (hardcoded for development)
#define WIFI_SSID      "ul_iot"
#define WIFI_PASSWORD  ""

// Maximum retry attempts before giving up
#define WIFI_MAXIMUM_RETRY    10

/**
 * @brief Initialize WiFi in station mode
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_manager_init(void);

/**
 * @brief Wait for WiFi connection
 * @param timeout_ms Maximum time to wait in milliseconds
 * @return true if connected, false if timeout
 */
bool wifi_manager_wait_connected(uint32_t timeout_ms);

/**
 * @brief Check if WiFi is currently connected
 * @return true if connected
 */
bool wifi_manager_is_connected(void);

#endif // WIFI_MANAGER_H
