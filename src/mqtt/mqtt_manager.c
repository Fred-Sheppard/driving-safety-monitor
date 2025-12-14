#include "mqtt_manager.h"
#include "mqtt_internal.h"
#include "config.h"

#include "freertos/FreeRTOS.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "esp_mac.h"
#include <string.h>

static const char *TAG = "mqtt";

// Global state (shared via mqtt_internal.h)
char g_device_id[DEVICE_ID_LEN] = {0};
esp_mqtt_client_handle_t g_mqtt_client = NULL;
bool g_mqtt_connected = false;
bool g_status_requested = false;

// Device-specific command topic
static char s_commands_topic[64];

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "Connected to broker");
        g_mqtt_connected = true;
        esp_mqtt_client_subscribe(g_mqtt_client, s_commands_topic, MQTT_QOS_COMMANDS);
        ESP_LOGI(TAG, "Subscribed to %s", s_commands_topic);
        g_status_requested = true;
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "Disconnected from broker");
        g_mqtt_connected = false;
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGD(TAG, "Message published, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        if (event->topic_len == strlen(s_commands_topic) &&
            strncmp(event->topic, s_commands_topic, event->topic_len) == 0)
        {
            mqtt_handle_command(event->data, event->data_len);
        }
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT error occurred");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
        {
            ESP_LOGE(TAG, "Transport error: %s",
                     strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;

    default:
        ESP_LOGD(TAG, "Other event id: %d", event->event_id);
        break;
    }
}

esp_err_t mqtt_manager_init(void)
{
    static char client_id[32];
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    // Set global device ID (just the MAC portion)
    snprintf(g_device_id, sizeof(g_device_id), "%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // Client ID includes "driving-" prefix
    snprintf(client_id, sizeof(client_id), "driving-%s", g_device_id);

    // Build device-specific command topic
    snprintf(s_commands_topic, sizeof(s_commands_topic), "driving/commands/%s", g_device_id);

    ESP_LOGI(TAG, "Device ID: %s", g_device_id);

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
        .credentials.client_id = client_id,
        .session.keepalive = 60,
        .session.disable_clean_session = true,
        .network.reconnect_timeout_ms = 5000,
        .network.timeout_ms = 10000,
    };

    g_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (g_mqtt_client == NULL)
    {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return ESP_FAIL;
    }

    ESP_ERROR_CHECK(esp_mqtt_client_register_event(
        g_mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL));

    ESP_LOGI(TAG, "MQTT client initialized");
    return ESP_OK;
}

esp_err_t mqtt_manager_start(void)
{
    return esp_mqtt_client_start(g_mqtt_client);
}

bool mqtt_manager_is_connected(void)
{
    return g_mqtt_connected;
}
