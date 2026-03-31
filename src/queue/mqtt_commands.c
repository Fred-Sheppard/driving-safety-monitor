#include "serialize.h"
#include "mqtt_internal.h"
#include "config.h"
#include "message_types.h"
#include "queue/ring_buffer.h"
#include "queue/ring_buffer_utils.h"

#include "mqtt_client.h"
#include "esp_log.h"

#include <string.h>
#include <stdlib.h>

static const char *TAG = "mqtt_cmd";

#define CMD_BUFFER_SIZE 128
static char s_cmd_buffer[CMD_BUFFER_SIZE];

static const char *json_get_string(const char *json, const char *key, int *len)
{
    char pattern[32];
    snprintf(pattern, sizeof(pattern), "\"%s\":\"", key);

    const char *start = strstr(json, pattern);
    if (!start)
        return NULL;

    start += strlen(pattern);
    const char *end = strchr(start, '"');
    if (!end)
        return NULL;

    *len = end - start;
    return start;
}

static bool json_get_float(const char *json, const char *key, float *out)
{
    char pattern[32];
    snprintf(pattern, sizeof(pattern), "\"%s\":", key);

    const char *start = strstr(json, pattern);
    if (!start)
        return false;

    start += strlen(pattern);

    // Skip whitespace
    while (*start == ' ')
        start++;

    *out = strtof(start, NULL);
    return true;
}

static bool str_eq(const char *s, int len, const char *match)
{
    return (int)strlen(match) == len && strncmp(s, match, len) == 0;
}

static void handle_get_status(void)
{
    mqtt_command_t cmd = {.type = MQTT_CMD_GET_STATUS};
    if (ring_buffer_push_back_with_full_log(mqtt_command_queue, &cmd,
                                            "Command queue full, overwrote oldest command"))
    {
        ESP_LOGI(TAG, "Status request queued");
    }
    else
    {
        ESP_LOGE(TAG, "Failed to queue status request");
    }
}

static void handle_set_threshold(const char *json)
{
    int type_len;
    const char *type_str = json_get_string(json, "type", &type_len);
    float value;

    if (!type_str || !json_get_float(json, "value", &value))
    {
        ESP_LOGE(TAG, "set_threshold missing type or value");
        return;
    }

    mqtt_command_t cmd = {
        .type = MQTT_CMD_SET_THRESHOLD,
        .data.set_threshold.value = value};

    if (str_eq(type_str, type_len, "crash"))
        cmd.data.set_threshold.threshold = THRESHOLD_CRASH;
    else if (str_eq(type_str, type_len, "braking"))
        cmd.data.set_threshold.threshold = THRESHOLD_BRAKING;
    else if (str_eq(type_str, type_len, "accel"))
        cmd.data.set_threshold.threshold = THRESHOLD_ACCEL;
    else if (str_eq(type_str, type_len, "cornering"))
        cmd.data.set_threshold.threshold = THRESHOLD_CORNERING;
    else
    {
        ESP_LOGW(TAG, "Unknown threshold type: %.*s", type_len, type_str);
        return;
    }

    if (ring_buffer_push_back_with_full_log(mqtt_command_queue, &cmd,
                                            "Command queue full, overwrote oldest command"))
    {
        ESP_LOGI(TAG, "Set %.*s threshold to %.1f", type_len, type_str, value);
    }
    else
    {
        ESP_LOGE(TAG, "Failed to queue command");
    }
}

// --- Public API ---

void mqtt_publish_status(const threshold_status_t *status)
{
    const char *json = serialize_status(status);
    if (!json)
    {
        return;
    }

    int msg_id = esp_mqtt_client_publish(
        g_mqtt_client, MQTT_TOPIC_STATUS, json, 0, MQTT_QOS_STATUS, 0);

    if (msg_id >= 0)
    {
        ESP_LOGI(TAG, "Thresholds: crash=%.1f braking=%.1f accel=%.1f cornering=%.1f",
                 status->crash, status->braking, status->accel, status->cornering);
    }
    else
    {
        ESP_LOGE(TAG, "Failed to publish status");
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

    int cmd_len;
    const char *cmd_str = json_get_string(s_cmd_buffer, "cmd", &cmd_len);
    if (!cmd_str)
    {
        ESP_LOGW(TAG, "Missing cmd field");
        return;
    }

    if (str_eq(cmd_str, cmd_len, "get_status"))
    {
        handle_get_status();
    }
    else if (str_eq(cmd_str, cmd_len, "set_threshold"))
    {
        handle_set_threshold(s_cmd_buffer);
    }
    else
    {
        ESP_LOGW(TAG, "Unknown command: %.*s", cmd_len, cmd_str);
    }
}