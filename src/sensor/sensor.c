#include "sensor.h"
#include "periph/i2c/i2c_bus.h"
#include "periph/i2c/mpu6050/mpu6050.h"
#include "config.h"
#include "message_types.h"
#include "queue/ring_buffer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "trace/trace.h"
#include <math.h>

static const char *TAG = "sensor";

#ifdef MOCK_SENSOR_DATA
#define MOCK_LATERAL_AMPLITUDE 0.1f
#define MOCK_FORWARD_AMPLITUDE 0.2f
#define MOCK_GRAVITY_BASE 9.81f
#define MOCK_GRAVITY_VARIATION 0.05f
#define MOCK_BRAKE_EVENT_TICKS 500
#define MOCK_BRAKE_FORCE -2.0f

static sensor_reading_t read_mock_imu(void);
#endif

esp_err_t sensor_i2c_init(void)
{
#ifdef MOCK_SENSOR_DATA
    return ESP_OK;
#endif
    ESP_ERROR_CHECK(i2c_bus_init());
    return mpu6050_init();
}

sensor_reading_t read_imu(void)
{
#ifndef MOCK_SENSOR_DATA
    float ax, ay, az;
    mpu6050_read_accel(&ax, &ay, &az);
    return (sensor_reading_t){ax, ay, az};
#else
    return read_mock_imu();
#endif
}

#ifdef MOCK_SENSOR_DATA
static sensor_reading_t read_mock_imu(void)
{
    static uint32_t tick = 0;
    tick++;

    sensor_reading_t r = {
        .x = MOCK_LATERAL_AMPLITUDE * sinf(tick * 0.05f),
        .y = MOCK_FORWARD_AMPLITUDE * sinf(tick * 0.02f),
        .z = MOCK_GRAVITY_BASE + MOCK_GRAVITY_VARIATION * sinf(tick * 0.1f)};

    if (tick % MOCK_BRAKE_EVENT_TICKS == 0)
    {
        r.y = -DEFAULT_HARSH_BRAKING_THRESHOLD_G - 1.0f;
        ESP_LOGI(TAG, "Mock brake event");
    }
    return r;
}
#endif

void sensor_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Sensor task running");

    while (1)
    {
        TRACE_TASK_RUN(TAG);
        sensor_reading_t r = read_imu();

        if (!ring_buffer_push(sensor_rb, &r))
            ESP_LOGW(TAG, "sensor_rb: failed to push sensor reading");

        vTaskDelay(pdMS_TO_TICKS(SENSOR_INTERVAL_MS));
    }
}
