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

esp_err_t wifi_manager_init(void);
bool wifi_manager_wait_connected(uint32_t timeout_ms);
bool wifi_manager_is_connected(void);
esp_err_t wifi_manager_start_scan(void);
// Call after scan completes
uint16_t wifi_manager_get_scan_results(wifi_scan_result_t *results, uint16_t max_results);
bool wifi_manager_scan_done(void);
esp_err_t wifi_manager_connect(const char *ssid, const char *password);
esp_err_t wifi_manager_disconnect(void);
// ssid_buf must be at least WIFI_SSID_MAX_LEN
bool wifi_manager_get_ssid(char *ssid_buf);

#ifdef __cplusplus
}
#endif

#endif // WIFI_MANAGER_H
