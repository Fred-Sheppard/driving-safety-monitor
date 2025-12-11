#include "crash.h"
#include "config.h"
#include "message_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <math.h>

static const char *TAG = "crash";

// Dynamic threshold (can be updated by server commands)
static float crash_threshold_g = DEFAULT_CRASH_THRESHOLD_G;

void crash_detection_init(void)
{
    crash_threshold_g = DEFAULT_CRASH_THRESHOLD_G;
}

void crash_set_threshold(float threshold_g)
{
    crash_threshold_g = threshold_g;
    ESP_LOGI(TAG, "Crash threshold set to %.2f g", threshold_g);
}

float crash_get_threshold(void)
{
    return crash_threshold_g;
}

static float calculate_accel_magnitude(const sensor_reading_t *data)
{
    return sqrtf(data->x * data->x +
                 data->y * data->y +
                 data->z * data->z);
}

bool detect_crash(const sensor_reading_t *data)
{
    float accel_magnitude = calculate_accel_magnitude(data);
    return accel_magnitude >= crash_threshold_g;
}

void handle_crash(const sensor_reading_t *data)
{
    mqtt_message_t msg = {
        .type = MSG_CRASH,
        .data.crash = {
            .timestamp = xTaskGetTickCount(),
            .accel_magnitude = sqrtf(data->x * data->x +
                                     data->y * data->y +
                                     data->z * data->z)}};

    // Send to mqtt_queue for immediate transmission
    if (xQueueSend(mqtt_queue, &msg, 0) != pdTRUE)
    {
        ESP_LOGW(TAG, "Failed to queue crash alert");
    }
    else
    {
        ESP_LOGI(TAG, "Crash detected! Magnitude: %.2f g",
                 msg.data.crash.accel_magnitude);
    }
}
