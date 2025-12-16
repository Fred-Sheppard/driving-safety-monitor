#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WIFI_MAX_SCAN_RESULTS 15
#define WIFI_SSID_MAX_LEN 33

typedef struct {
    char ssid[WIFI_SSID_MAX_LEN];
    int8_t rssi;
    uint8_t authmode;
} wifi_scan_result_t;

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

/**
 * @brief Start async WiFi scan
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_start_scan(void);

/**
 * @brief Get scan results (call after scan completes)
 * @param results Array to store results
 * @param max_results Maximum number of results to return
 * @return Number of networks found
 */
uint16_t wifi_manager_get_scan_results(wifi_scan_result_t *results, uint16_t max_results);

/**
 * @brief Check if scan is complete
 * @return true if scan done
 */
bool wifi_manager_scan_done(void);

/**
 * @brief Connect to a new WiFi network
 * @param ssid Network SSID
 * @param password Network password
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_connect(const char *ssid, const char *password);

/**
 * @brief Disconnect from current WiFi network
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_disconnect(void);

/**
 * @brief Get the SSID of the currently connected network
 * @param ssid_buf Buffer to store SSID (must be at least WIFI_SSID_MAX_LEN)
 * @return true if connected and SSID retrieved, false otherwise
 */
bool wifi_manager_get_ssid(char *ssid_buf);

#ifdef __cplusplus
}
#endif

#endif // WIFI_MANAGER_H
