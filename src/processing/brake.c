#include "brake.h"
#include "config.h"
#include "message_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "brake";

// Dynamic threshold (can be updated by server commands)
static float harsh_braking_threshold_g = DEFAULT_HARSH_BRAKING_THRESHOLD_G;

void brake_detection_init(void)
{
    harsh_braking_threshold_g = DEFAULT_HARSH_BRAKING_THRESHOLD_G;
}

void brake_set_threshold(float threshold_g)
{
    harsh_braking_threshold_g = threshold_g;
    ESP_LOGI(TAG, "Harsh braking threshold set to %.2f g", threshold_g);
}

float brake_get_threshold(void)
{
    return harsh_braking_threshold_g;
}

bool detect_harsh_braking(const sensor_reading_t *data)
{
    // Harsh braking produces negative Y-axis acceleration (deceleration)
    return data->y <= -harsh_braking_threshold_g;
}

void handle_harsh_braking(const sensor_reading_t *data)
{
    mqtt_message_t msg = {
        .type = MSG_WARNING,
        .data.warning = {
            .event = WARNING_HARSH_BRAKING,
            .timestamp = xTaskGetTickCount(),
            .accel_x = data->x,
            .accel_y = data->y}};

    // Send to mqtt_queue for immediate transmission
    if (xQueueSend(mqtt_queue, &msg, 0) != pdTRUE)
    {
        ESP_LOGW(TAG, "Failed to queue harsh braking alert");
    }
    else
    {
        ESP_LOGI(TAG, "Harsh braking detected! Y-accel: %.2f g", data->y);
    }
}
