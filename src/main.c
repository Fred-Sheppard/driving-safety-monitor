#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "config.h"
#include "message_types.h"
#include "wifi/wifi_manager.h"
#include "mqtt/mqtt_manager.h"
#include "processing/process.h"
#include "sensor/sensor.h"
#include "display/display_manager.hpp"
#include "trace/trace.h"
#include "queue/bidir_queue.h"
#include "queue/ring_buffer.h"
#include "watchdog/watchdog.h"

static const char *TAG = "main";

ring_buffer_t *sensor_rb = NULL;
ring_buffer_t *batch_rb = NULL;
ring_buffer_t *mqtt_rb = NULL;
void app_main(void)
{
    ESP_LOGI(TAG, "Driving Safety Monitor starting...");

    ESP_ERROR_CHECK(watchdog_init(WATCHDOG_TIMEOUT_SECONDS));

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    if (sensor_i2c_init() != ESP_OK)
    {
        ESP_LOGE(TAG, "I2C init failed");
        return;
    }
    ESP_LOGI(TAG, "I2C init successful");

    mqtt_rb = ring_buffer_create(MQTT_QUEUE_SIZE, sizeof(mqtt_message_t));
    batch_rb = ring_buffer_create(BATCH_QUEUE_SIZE, sizeof(sensor_batch_t));
    sensor_rb = ring_buffer_create(SENSOR_QUEUE_SIZE, sizeof(sensor_reading_t));
    bidir_queue_init();
    if (mqtt_rb == NULL || batch_rb == NULL || sensor_rb == NULL)
    {
        ESP_LOGE(TAG, "Failed to create ring buffers");
        return;
    }
    ESP_LOGI(TAG, "Ring buffers created successfully");

    ESP_ERROR_CHECK(wifi_manager_init());

    ESP_ERROR_CHECK(mqtt_manager_init());
    ESP_ERROR_CHECK(mqtt_manager_start());
    ESP_LOGI(TAG, "MQTT client initialized (waiting for WiFi)");

    // TODO: Log a warning if display is not connected
    display_init();

    // Large stack due to JSON serialization of batches
    xTaskCreate(mqtt_task, "mqtt", 16384, NULL, MQTT_TASK_PRIORITY, NULL);
    xTaskCreate(processing_task, "process", 4096, NULL, PROCESSING_TASK_PRIORITY, NULL);
    xTaskCreate(sensor_task, "sensor", 4096, NULL, SENSOR_TASK_PRIORITY, NULL);
    xTaskCreate(displayTask, "display", 8192, NULL, SCREEN_TASK_PRIORITY, NULL);

    trace_init();
    trace_start_stats_task();

    ESP_LOGI(TAG, "Tasks created, system running");
}