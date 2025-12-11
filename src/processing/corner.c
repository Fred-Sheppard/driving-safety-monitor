#include "corner.h"
#include "config.h"
#include "message_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <math.h>

static const char *TAG = "corner";

// Dynamic threshold (can be updated by server commands)
static float harsh_cornering_threshold_g = DEFAULT_HARSH_CORNERING_THRESHOLD_G;

void corner_detection_init(void)
{
    harsh_cornering_threshold_g = DEFAULT_HARSH_CORNERING_THRESHOLD_G;
}

void corner_set_threshold(float threshold_g)
{
    harsh_cornering_threshold_g = threshold_g;
    ESP_LOGI(TAG, "Harsh cornering threshold set to %.2f g", threshold_g);
}

float corner_get_threshold(void)
{
    return harsh_cornering_threshold_g;
}

bool detect_harsh_cornering(const sensor_reading_t *data)
{
    // Harsh cornering produces lateral X-axis acceleration (left or right)
    return fabsf(data->x) >= harsh_cornering_threshold_g;
}

void handle_harsh_cornering(const sensor_reading_t *data)
{
    mqtt_message_t msg = {
        .type = MSG_WARNING,
        .data.warning = {
            .event = WARNING_HARSH_CORNERING,
            .timestamp = xTaskGetTickCount(),
            .accel_x = data->x,
            .accel_y = data->y}};

    // Send to mqtt_queue for immediate transmission
    if (xQueueSend(mqtt_queue, &msg, 0) != pdTRUE)
    {
        ESP_LOGW(TAG, "Failed to queue harsh cornering alert");
    }
    else
    {
        ESP_LOGI(TAG, "Harsh cornering detected! X-accel: %.2f g", data->x);
    }
}
