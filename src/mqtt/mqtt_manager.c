#include "mqtt_manager.h"
#include "serialize.h"
#include "config.h"
#include "message_types.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "mqtt_manager";

static esp_mqtt_client_handle_t s_mqtt_client = NULL;
static bool s_mqtt_connected = false;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data);

esp_err_t mqtt_manager_init(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address = {
                .uri = MQTT_BROKER_URI,
            },
        },
        .session = {
            .keepalive = 60,
        },
        .network = {
            .reconnect_timeout_ms = 5000,
            .timeout_ms = 10000,
        },
    };

    s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (s_mqtt_client == NULL)
    {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return ESP_FAIL;
    }

    ESP_ERROR_CHECK(esp_mqtt_client_register_event(
        s_mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL));

    ESP_LOGI(TAG, "MQTT client initialized");
    return ESP_OK;
}

esp_err_t mqtt_manager_start(void)
{
    return esp_mqtt_client_start(s_mqtt_client);
}

bool mqtt_manager_is_connected(void)
{
    return s_mqtt_connected;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT connected to broker");
        s_mqtt_connected = true;
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "MQTT disconnected from broker");
        s_mqtt_connected = false;
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGD(TAG, "Message published, msg_id=%d", event->msg_id);
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
        ESP_LOGD(TAG, "Other MQTT event id: %d", event->event_id);
        break;
    }
}

void mqtt_task(void *pvParameters)
{
    (void)pvParameters;
    mqtt_message_t alert_msg;
    sensor_batch_t batch;
    const char *json_payload = NULL;

    ESP_LOGI(TAG, "mqtt_task started");

    while (1)
    {
        // Wait for MQTT connection before processing
        if (!mqtt_manager_is_connected())
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        // Priority 1: Process ALL pending alerts (non-blocking)
        while (xQueueReceive(mqtt_queue, &alert_msg, 0) == pdTRUE)
        {
            json_payload = serialize_alert(&alert_msg);
            if (json_payload == NULL)
            {
                ESP_LOGE(TAG, "Failed to serialize alert");
                continue;
            }

            int msg_id = esp_mqtt_client_publish(
                s_mqtt_client,
                MQTT_TOPIC_ALERTS,
                json_payload,
                0,
                MQTT_QOS_ALERTS,
                0);

            if (msg_id >= 0)
            {
                ESP_LOGI(TAG, "Alert published: %s",
                         alert_msg.type == MSG_CRASH ? "CRASH" : "WARNING");
            }
            else
            {
                ESP_LOGE(TAG, "Failed to publish alert");
            }
        }

        // Priority 2: Check for batch telemetry (blocking with 1s timeout)
        if (xQueueReceive(batch_queue, &batch, pdMS_TO_TICKS(1000)) == pdTRUE)
        {
            json_payload = serialize_batch(&batch);
            if (json_payload == NULL)
            {
                ESP_LOGE(TAG, "Failed to serialize batch");
                continue;
            }

            int msg_id = esp_mqtt_client_publish(
                s_mqtt_client,
                MQTT_TOPIC_TELEMETRY,
                json_payload,
                0,
                MQTT_QOS_TELEMETRY,
                0);

            if (msg_id >= 0)
            {
                ESP_LOGI(TAG, "Batch published, samples=%d", batch.sample_count);
            }
            else
            {
                ESP_LOGE(TAG, "Failed to publish batch");
            }
        }
    }
}
