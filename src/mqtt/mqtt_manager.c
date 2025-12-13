#include "mqtt_manager.h"
#include "serialize.h"
#include "config.h"
#include "message_types.h"
#include "queue/bidir_queue.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "trace/trace.h"
#include "cJSON.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "mqtt_manager";

static esp_mqtt_client_handle_t s_mqtt_client = NULL;
static bool s_mqtt_connected = false;
static bool s_status_requested = false;  // Flag to request initial status on connect

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data);
static void handle_command_message(const char *data, int data_len);
static void publish_status(const threshold_status_t *status);

esp_err_t mqtt_manager_init(void)
{
    // Build unique client ID from MAC address
    static char client_id[32];
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(client_id, sizeof(client_id), "driving-%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    ESP_LOGI(TAG, "MQTT client ID: %s", client_id);

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address = {
                .uri = MQTT_BROKER_URI,
            },
        },
        .credentials = {
            .client_id = client_id,
        },
        .session = {
            .keepalive = 60,
            .disable_clean_session = true,
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
        // Subscribe to commands topic
        esp_mqtt_client_subscribe(s_mqtt_client, MQTT_TOPIC_COMMANDS, MQTT_QOS_COMMANDS);
        ESP_LOGI(TAG, "Subscribed to %s", MQTT_TOPIC_COMMANDS);
        // Request status publish on next mqtt_task iteration
        s_status_requested = true;
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "MQTT disconnected from broker");
        s_mqtt_connected = false;
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGD(TAG, "Message published, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        if (event->topic_len == strlen(MQTT_TOPIC_COMMANDS) &&
            strncmp(event->topic, MQTT_TOPIC_COMMANDS, event->topic_len) == 0)
        {
            handle_command_message(event->data, event->data_len);
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
        ESP_LOGD(TAG, "Other MQTT event id: %d", event->event_id);
        break;
    }
}

static void publish_status(const threshold_status_t *status)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "crash", status->crash);
    cJSON_AddNumberToObject(root, "braking", status->braking);
    cJSON_AddNumberToObject(root, "accel", status->accel);
    cJSON_AddNumberToObject(root, "cornering", status->cornering);

    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (json_str)
    {
        int msg_id = esp_mqtt_client_publish(
            s_mqtt_client,
            MQTT_TOPIC_STATUS,
            json_str,
            0,
            MQTT_QOS_STATUS,
            0);

        if (msg_id >= 0)
        {
            ESP_LOGI(TAG, "Status published: crash=%.1f braking=%.1f accel=%.1f cornering=%.1f",
                     status->crash, status->braking, status->accel, status->cornering);
        }
        else
        {
            ESP_LOGE(TAG, "Failed to publish status");
        }
        free(json_str);
    }
}

void mqtt_task(void *pvParameters)
{
    (void)pvParameters;
    mqtt_message_t alert_msg;
    sensor_batch_t batch;
    bidir_message_t bidir_msg;
    const char *json_payload = NULL;

    ESP_LOGI(TAG, "mqtt_task started");

    while (1)
    {
        TRACE_TASK_RUN(TAG);

        // Wait for MQTT connection before processing
        if (!mqtt_manager_is_connected())
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        // On connect: request current status from processing task
        if (s_status_requested)
        {
            s_status_requested = false;
            bidir_message_t status_req = {
                .direction = BIDIR_INBOUND,
                .type = BIDIR_CMD_GET_STATUS
            };
            bidir_queue_push(&status_req);
            ESP_LOGI(TAG, "Requested initial status");
        }

        // Priority 0: Process outbound status responses from bidir queue
        while (bidir_queue_pop(BIDIR_OUTBOUND, &bidir_msg))
        {
            if (bidir_msg.type == BIDIR_RESP_STATUS)
            {
                publish_status(&bidir_msg.data.status);
            }
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

/**
 * Parse and handle incoming command message
 * Expected JSON: {"cmd":"set_threshold","type":"crash|braking|accel|cornering","value":10.0}
 */
static void handle_command_message(const char *data, int data_len)
{
    char *json_str = malloc(data_len + 1);
    if (!json_str)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for command");
        return;
    }
    memcpy(json_str, data, data_len);
    json_str[data_len] = '\0';

    cJSON *root = cJSON_Parse(json_str);
    free(json_str);

    if (!root)
    {
        ESP_LOGE(TAG, "Failed to parse command JSON");
        return;
    }

    cJSON *cmd_obj = cJSON_GetObjectItem(root, "cmd");

    if (cmd_obj && strcmp(cmd_obj->valuestring, "get_status") == 0)
    {
        // Request status
        bidir_message_t msg = {
            .direction = BIDIR_INBOUND,
            .type = BIDIR_CMD_GET_STATUS
        };
        bidir_queue_push(&msg);
        ESP_LOGI(TAG, "Status request queued");
    }
    else if (cmd_obj && strcmp(cmd_obj->valuestring, "set_threshold") == 0)
    {
        cJSON *type_obj = cJSON_GetObjectItem(root, "type");
        cJSON *value_obj = cJSON_GetObjectItem(root, "value");

        if (!type_obj || !value_obj)
        {
            ESP_LOGE(TAG, "set_threshold missing type or value");
            cJSON_Delete(root);
            return;
        }

        bidir_message_t msg = {
            .direction = BIDIR_INBOUND,
            .type = BIDIR_CMD_SET_THRESHOLD,
            .data.set_threshold.value = (float)value_obj->valuedouble
        };

        const char *type = type_obj->valuestring;
        if (strcmp(type, "crash") == 0)
            msg.data.set_threshold.threshold = THRESHOLD_CRASH;
        else if (strcmp(type, "braking") == 0)
            msg.data.set_threshold.threshold = THRESHOLD_BRAKING;
        else if (strcmp(type, "accel") == 0)
            msg.data.set_threshold.threshold = THRESHOLD_ACCEL;
        else if (strcmp(type, "cornering") == 0)
            msg.data.set_threshold.threshold = THRESHOLD_CORNERING;
        else
        {
            ESP_LOGW(TAG, "Unknown threshold type: %s", type);
            cJSON_Delete(root);
            return;
        }

        if (bidir_queue_push(&msg))
        {
            ESP_LOGI(TAG, "Command queued: set %s threshold to %.1f", type, msg.data.set_threshold.value);
        }
        else
        {
            ESP_LOGW(TAG, "Bidir queue full");
        }
    }
    else
    {
        ESP_LOGW(TAG, "Unknown command");
    }

    cJSON_Delete(root);
}
