#include "serialize.h"
#include "mqtt_internal.h"
#include "config.h"
#include "message_types.h"
#include "esp_log.h"
#include <stdio.h>

static const char *TAG = "serialize";

// Static buffer for alert JSON (alerts are small, fixed size)
#define ALERT_BUFFER_SIZE 256
static char s_alert_buffer[ALERT_BUFFER_SIZE];

// Static buffer for batch JSON
// Estimate: header ~50 bytes + 500 samples * ~30 bytes each = ~15KB
#define BATCH_BUFFER_SIZE (50 + (LOG_BATCH_SIZE * 35))
static char s_batch_buffer[BATCH_BUFFER_SIZE];

const char *serialize_alert(const mqtt_message_t *msg) {
    int len = 0;

    if (msg->type == MSG_WARNING) {
        len = snprintf(s_alert_buffer, ALERT_BUFFER_SIZE,
            "{\"dev\":\"%s\",\"type\":\"warning\",\"event\":\"%s\",\"ts\":%lu,\"x\":%.3f,\"y\":%.3f}",
            g_device_id,
            warning_event_to_string(msg->data.warning.event),
            (unsigned long)msg->data.warning.timestamp,
            msg->data.warning.accel_x,
            msg->data.warning.accel_y);
    } else if (msg->type == MSG_CRASH) {
        len = snprintf(s_alert_buffer, ALERT_BUFFER_SIZE,
            "{\"dev\":\"%s\",\"type\":\"crash\",\"ts\":%lu,\"mag\":%.3f}",
            g_device_id,
            (unsigned long)msg->data.crash.timestamp,
            msg->data.crash.accel_magnitude);
    } else {
        return NULL;
    }

    if (len < 0 || len >= ALERT_BUFFER_SIZE) {
        ESP_LOGE(TAG, "Alert buffer overflow");
        return NULL;
    }

    return s_alert_buffer;
}

const char *serialize_batch(const sensor_batch_t *batch) {
    char *ptr = s_batch_buffer;
    char *end = s_batch_buffer + BATCH_BUFFER_SIZE;
    int written;

    written = snprintf(ptr, end - ptr,
        "{\"dev\":\"%s\",\"ts\":%lu,\"rate\":%u,\"n\":%u,\"d\":[",
        g_device_id,
        (unsigned long)batch->batch_start_timestamp,
        batch->sample_rate_hz,
        batch->sample_count);

    if (written < 0 || ptr + written >= end) {
        ESP_LOGE(TAG, "Batch header overflow");
        return NULL;
    }
    ptr += written;

    for (uint16_t i = 0; i < batch->sample_count; i++) {
        written = snprintf(ptr, end - ptr,
            "%s[%.4f,%.4f,%.4f]",
            (i > 0) ? "," : "",
            batch->samples[i].x,
            batch->samples[i].y,
            batch->samples[i].z);

        if (written < 0 || ptr + written >= end) {
            ESP_LOGE(TAG, "Batch overflow at sample %u", i);
            return NULL;
        }
        ptr += written;
    }

    written = snprintf(ptr, end - ptr, "]}");
    if (written < 0 || ptr + written >= end) {
        ESP_LOGE(TAG, "Batch close overflow");
        return NULL;
    }

    return s_batch_buffer;
}

// Static buffer for status JSON
#define STATUS_BUFFER_SIZE 128
static char s_status_buffer[STATUS_BUFFER_SIZE];

const char *serialize_status(const threshold_status_t *status)
{
    int len = snprintf(s_status_buffer, STATUS_BUFFER_SIZE,
                       "{\"dev\":\"%s\",\"crash\":%.1f,\"braking\":%.1f,\"accel\":%.1f,\"cornering\":%.1f}",
                       g_device_id,
                       status->crash,
                       status->braking,
                       status->accel,
                       status->cornering);

    if (len < 0 || len >= STATUS_BUFFER_SIZE)
    {
        ESP_LOGE(TAG, "Status buffer overflow");
        return NULL;
    }

    return s_status_buffer;
}