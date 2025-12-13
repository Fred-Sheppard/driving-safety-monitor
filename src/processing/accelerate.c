#include "accelerate.h"
#include "config.h"
#include "message_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "accelerate";

// Dynamic threshold (can be updated by server commands)
static float harsh_accel_threshold_g = DEFAULT_HARSH_ACCEL_THRESHOLD_G;

void accel_detection_init(void)
{
    harsh_accel_threshold_g = DEFAULT_HARSH_ACCEL_THRESHOLD_G;
}

void accel_set_threshold(float threshold_g)
{
    harsh_accel_threshold_g = threshold_g;
    ESP_LOGI(TAG, "Harsh acceleration threshold set to %.2f g", threshold_g);
}

float accel_get_threshold(void)
{
    return harsh_accel_threshold_g;
}

bool detect_harsh_acceleration(const sensor_reading_t *data)
{
    // Harsh acceleration produces positive Y-axis acceleration
    return data->y >= harsh_accel_threshold_g;
}

void handle_harsh_acceleration(const sensor_reading_t *data)
{
    mqtt_message_t msg = {
        .type = MSG_WARNING,
        .data.warning = {
            .event = WARNING_HARSH_ACCEL,
            .timestamp = xTaskGetTickCount(),
            .accel_x = data->x,
            .accel_y = data->y}};

    // Send to mqtt_queue for immediate transmission
    if (xQueueSend(mqtt_queue, &msg, 0) != pdTRUE)
    {
        ESP_LOGW(TAG, "mqtt_queue: failed to queue harsh acceleration alert. Queue full");
    }
    else
    {
        ESP_LOGI(TAG, "Harsh acceleration detected! Y-accel: %.2f g", data->y);
    }
}
