#include "wifi_manager.h"
#include "config.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"

static const char *TAG = "wifi_manager";

// Event bits for WiFi status
#define WIFI_CONNECTED_BIT    BIT0
#define WIFI_FAIL_BIT         BIT1

static EventGroupHandle_t s_wifi_event_group = NULL;
static int s_retry_num = 0;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t *disconn = (wifi_event_sta_disconnected_t *)event_data;

        // Clear connected bit so mqtt_manager knows we're offline
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

        s_retry_num++;
        ESP_LOGW(TAG, "Disconnected (reason: %d), reconnecting... (attempt %d)",
                 disconn->reason, s_retry_num);

        // Always retry - never give up
        vTaskDelay(pdMS_TO_TICKS(1000)); // Brief delay before retry
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupClearBits(s_wifi_event_group, WIFI_FAIL_BIT);
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_scan_and_print(void) {
    uint16_t ap_count = 0;
    uint16_t max_aps = 20;
    wifi_ap_record_t *ap_list = malloc(sizeof(wifi_ap_record_t) * max_aps);
    if (ap_list == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for scan");
        return;
    }

    ESP_LOGI(TAG, "Scanning for WiFi networks...");
    esp_wifi_scan_start(NULL, true);  // Blocking scan
    esp_wifi_scan_get_ap_num(&ap_count);

    if (ap_count > max_aps) {
        ap_count = max_aps;
    }

    esp_wifi_scan_get_ap_records(&ap_count, ap_list);

    ESP_LOGI(TAG, "Found %d networks:", ap_count);
    for (int i = 0; i < ap_count; i++) {
        ESP_LOGI(TAG, "  %d. SSID: %-32s | RSSI: %d | Channel: %d",
                 i + 1, ap_list[i].ssid, ap_list[i].rssi, ap_list[i].primary);
    }

    free(ap_list);
}

esp_err_t wifi_manager_init(void) {
    // Create event group for status signaling
    s_wifi_event_group = xEventGroupCreate();
    if (s_wifi_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create event group");
        return ESP_FAIL;
    }

    // Initialize TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());

    // Create default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create default WiFi station interface
    esp_netif_create_default_wifi_sta();

    // Initialize WiFi with default config
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Set station mode and start WiFi for scanning
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Scan and print available networks
    wifi_scan_and_print();

    // Register event handlers
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID,
        &wifi_event_handler, NULL, &instance_any_id));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP,
        &wifi_event_handler, NULL, &instance_got_ip));

    // Configure WiFi
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    ESP_LOGI(TAG, "WiFi initialization complete, connecting to %s", WIFI_SSID);

    // Trigger connection (WiFi already started for scanning)
    esp_wifi_connect();

    return ESP_OK;
}

bool wifi_manager_wait_connected(uint32_t timeout_ms) {
    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        pdMS_TO_TICKS(timeout_ms));

    if (bits & WIFI_CONNECTED_BIT) {
        return true;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "WiFi connection failed");
        return false;
    } else {
        ESP_LOGE(TAG, "WiFi connection timeout");
        return false;
    }
}

bool wifi_manager_is_connected(void) {
    if (s_wifi_event_group == NULL) {
        return false;
    }
    EventBits_t bits = xEventGroupGetBits(s_wifi_event_group);
    return (bits & WIFI_CONNECTED_BIT) != 0;
}
