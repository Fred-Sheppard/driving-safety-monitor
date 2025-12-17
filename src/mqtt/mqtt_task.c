#include "mqtt_manager.h"
#include "mqtt_internal.h"
#include "serialize.h"
#include "config.h"
#include "message_types.h"
#include "queue/ring_buffer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "trace/trace.h"
#include "watchdog/watchdog.h"

static const char *TAG = "mqtt_task";

static void process_mqtt_responses(void)
{
    mqtt_command_t response;
    while (ring_buffer_pop_front(mqtt_response_queue, &response))
    {
        if (response.type == MQTT_RESP_STATUS)
        {
            mqtt_publish_status(&response.data.status);
        }
    }
}

static void process_alerts(void)
{
    mqtt_message_t alert_msg;
    while (ring_buffer_pop_front(mqtt_rb, &alert_msg))
    {
        const char *json_payload = serialize_alert(&alert_msg);
        if (json_payload == NULL)
        {
            ESP_LOGE(TAG, "Failed to serialize alert");
            continue;
        }

        int msg_id = esp_mqtt_client_publish(
            g_mqtt_client, MQTT_TOPIC_ALERTS, json_payload, 0, MQTT_QOS_ALERTS, 0);

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
}

static void process_telemetry(void)
{
    sensor_batch_t batch;
    if (ring_buffer_pop_front(batch_rb, &batch))
    {
        const char *json_payload = serialize_batch(&batch);
        if (json_payload == NULL)
        {
            ESP_LOGE(TAG, "Failed to serialize batch");
            return;
        }

        int msg_id = esp_mqtt_client_publish(
            g_mqtt_client, MQTT_TOPIC_TELEMETRY, json_payload, 0, MQTT_QOS_TELEMETRY, 0);

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

static void request_initial_status(void)
{
    mqtt_command_t status_req = {
        .type = MQTT_CMD_GET_STATUS};
    bool was_full = false;
    if (ring_buffer_push_back(mqtt_command_queue, &status_req, &was_full))
    {
        if (was_full)
        {
            ESP_LOGW(TAG, "Command queue full, overwrote oldest command");
        }
        ESP_LOGI(TAG, "Requested initial status");
    }
    else
    {
        ESP_LOGE(TAG, "Failed to queue status request");
    }
}

void mqtt_task(void *pvParameters)
{
    (void)pvParameters;
    ESP_LOGI(TAG, "mqtt_task started");
    watchdog_register_task();

    while (1)
    {
        TRACE_TASK_RUN(TAG);
        watchdog_feed();

        if (!mqtt_manager_is_connected())
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        if (g_status_requested)
        {
            g_status_requested = false;
            request_initial_status();
        }

        process_mqtt_responses();
        process_alerts();
        process_telemetry();
    }
}
