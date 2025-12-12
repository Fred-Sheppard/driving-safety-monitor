#include "sensor.h"
#include "config.h"
#include "message_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <math.h>

static const char *TAG = "sensor";

inline sensor_reading_t read_imu(void)
{
    // TODO: Niall: Implement actual I2C read from IMU
    sensor_reading_t reading = {0, 0, 0};
    return reading;
}

#ifdef MOCK_SENSOR_DATA
static sensor_reading_t read_mock_imu(void)
{
    static uint32_t tick = 0;
    tick++;

    // Generate varying mock data that simulates driving
    // Base gravity on Z-axis, with some variation on X and Y
    sensor_reading_t reading = {
        .x = 0.1f * sinf(tick * 0.05f),        // Gentle lateral sway
        .y = 0.2f * sinf(tick * 0.02f),        // Gentle forward/back
        .z = 9.81f + 0.05f * sinf(tick * 0.1f) // Gravity with slight bounce
    };

    // 1000 ticks ~= 10 seconds
    if (tick % 1000 == 0)
    {
        reading.y = -9.0f; // crash
    }
    else if (tick % 500 == 0)
    {
        reading.y = 3.6f; // Moderate braking
    }

    return reading;
}
#endif

void sensor_task(void *pvParameters)
{
    (void)pvParameters;
    sensor_reading_t reading;

    ESP_LOGI(TAG, "Sensor task started (interval: %d ms)", SENSOR_INTERVAL_MS);

#ifdef MOCK_SENSOR_DATA
    ESP_LOGW(TAG, "Using MOCK sensor data");
#endif

    while (1)
    {
#ifdef MOCK_SENSOR_DATA
        reading = read_mock_imu();
#else
        reading = read_imu();
#endif

        // TODO: Full queue policy
        if (xQueueSend(sensor_queue, &reading, 0) != pdTRUE)
        {
            ESP_LOGW(TAG, "Sensor queue full, dropping sample");
        }

        vTaskDelay(pdMS_TO_TICKS(SENSOR_INTERVAL_MS));
    }
}
