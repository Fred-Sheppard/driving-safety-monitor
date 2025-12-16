#include "mqtt_manager.h"
#include "mqtt_internal.h"
#include "serialize.h"
#include "config.h"
#include "message_types.h"
#include "queue/bidir_queue.h"
#include "queue/ring_buffer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "trace/trace.h"

static const char *TAG = "mqtt_task";

static void process_bidir_outbound(void)
{
    bidir_message_t bidir_msg;
    while (bidir_queue_pop(BIDIR_OUTBOUND, &bidir_msg))
    {
        if (bidir_msg.type == BIDIR_RESP_STATUS)
        {
            mqtt_publish_status(&bidir_msg.data.status);
        }
    }
}

static void process_alerts(void)
{
    mqtt_message_t alert_msg;
    while (ring_buffer_pop(mqtt_rb, &alert_msg))
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
    if (ring_buffer_pop(batch_rb, &batch))
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
    bidir_message_t status_req = {
        .direction = BIDIR_INBOUND,
        .type = BIDIR_CMD_GET_STATUS
    };
    bidir_queue_push(&status_req);
    ESP_LOGI(TAG, "Requested initial status");
}

void mqtt_task(void *pvParameters)
{
    (void)pvParameters;
    ESP_LOGI(TAG, "mqtt_task started");

    while (1)
    {
        TRACE_TASK_RUN(TAG);

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

        process_bidir_outbound();
        process_alerts();
        process_telemetry();
    }
}
