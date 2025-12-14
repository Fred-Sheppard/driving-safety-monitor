#include "mqtt_internal.h"
#include "config.h"
#include "queue/bidir_queue.h"

#include "mqtt_client.h"
#include "esp_log.h"
#include "cJSON.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "mqtt_cmd";

// Static buffer for incoming commands (avoids heap allocation)
#define CMD_BUFFER_SIZE 128
static char s_cmd_buffer[CMD_BUFFER_SIZE];

void mqtt_publish_status(const threshold_status_t *status)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "dev", g_device_id);
    cJSON_AddNumberToObject(root, "crash", status->crash);
    cJSON_AddNumberToObject(root, "braking", status->braking);
    cJSON_AddNumberToObject(root, "accel", status->accel);
    cJSON_AddNumberToObject(root, "cornering", status->cornering);

    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (json_str)
    {
        int msg_id = esp_mqtt_client_publish(
            g_mqtt_client, MQTT_TOPIC_STATUS, json_str, 0, MQTT_QOS_STATUS, 0);

        if (msg_id >= 0)
        {
            ESP_LOGI(TAG, "Status: crash=%.1f braking=%.1f accel=%.1f cornering=%.1f",
                     status->crash, status->braking, status->accel, status->cornering);
        }
        else
        {
            ESP_LOGE(TAG, "Failed to publish status");
        }
        free(json_str);
    }
}

static void handle_get_status(void)
{
    bidir_message_t msg = {
        .direction = BIDIR_INBOUND,
        .type = BIDIR_CMD_GET_STATUS
    };
    bidir_queue_push(&msg);
    ESP_LOGI(TAG, "Status request queued");
}

static void handle_set_threshold(cJSON *root)
{
    cJSON *type_obj = cJSON_GetObjectItem(root, "type");
    cJSON *value_obj = cJSON_GetObjectItem(root, "value");

    if (!type_obj || !value_obj)
    {
        ESP_LOGE(TAG, "set_threshold missing type or value");
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
        return;
    }

    if (bidir_queue_push(&msg))
    {
        ESP_LOGI(TAG, "Set %s threshold to %.1f", type, msg.data.set_threshold.value);
    }
    else
    {
        ESP_LOGW(TAG, "Bidir queue full");
    }
}

void mqtt_handle_command(const char *data, int data_len)
{
    if (data_len >= CMD_BUFFER_SIZE)
    {
        ESP_LOGE(TAG, "Command too large: %d bytes", data_len);
        return;
    }

    memcpy(s_cmd_buffer, data, data_len);
    s_cmd_buffer[data_len] = '\0';

    cJSON *root = cJSON_Parse(s_cmd_buffer);

    if (!root)
    {
        ESP_LOGE(TAG, "Failed to parse command JSON");
        return;
    }

    cJSON *cmd_obj = cJSON_GetObjectItem(root, "cmd");
    if (!cmd_obj || !cmd_obj->valuestring)
    {
        ESP_LOGW(TAG, "Missing cmd field");
        cJSON_Delete(root);
        return;
    }

    if (strcmp(cmd_obj->valuestring, "get_status") == 0)
    {
        handle_get_status();
    }
    else if (strcmp(cmd_obj->valuestring, "set_threshold") == 0)
    {
        handle_set_threshold(root);
    }
    else
    {
        ESP_LOGW(TAG, "Unknown command: %s", cmd_obj->valuestring);
    }

    cJSON_Delete(root);
}
