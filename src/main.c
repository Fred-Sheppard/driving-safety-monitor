#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "config.h"
#include "message_types.h"
#include "wifi/wifi_manager.h"
#include "mqtt/mqtt_manager.h"
#include "processing/process.h"
#include "sensor/sensor.h"

static const char *TAG = "main";

// Queue handles (declared extern in message_types.h)
QueueHandle_t sensor_queue = NULL;
QueueHandle_t batch_queue = NULL;
QueueHandle_t mqtt_queue = NULL;
QueueHandle_t command_queue = NULL;

static void send_mock_data(void);

void app_main(void)
{
    ESP_LOGI(TAG, "Driving Safety Monitor starting...");

    // 1. Initialize NVS (required for WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    // 1.5. (cba changing all)
    if (sensor_i2c_init() != ESP_OK) {
        ESP_LOGE(TAG, "I2C init failed");
        return;
    }
    ESP_LOGI(TAG, "I2C init successful");
    // 2. Create queues
    mqtt_queue = xQueueCreate(MQTT_QUEUE_SIZE, sizeof(mqtt_message_t));
    batch_queue = xQueueCreate(BATCH_QUEUE_SIZE, sizeof(sensor_batch_t));
    sensor_queue = xQueueCreate(SENSOR_QUEUE_SIZE, sizeof(sensor_reading_t));
    command_queue = xQueueCreate(COMMAND_QUEUE_SIZE, sizeof(uint8_t));

    if (mqtt_queue == NULL || batch_queue == NULL ||
        sensor_queue == NULL || command_queue == NULL)
    {
        ESP_LOGE(TAG, "Failed to create queues");
        return;
    }
    ESP_LOGI(TAG, "Queues created successfully");

    // 3. Initialize WiFi
    ESP_ERROR_CHECK(wifi_manager_init());

    // 4. Wait for WiFi connection (30 second timeout)
    if (!wifi_manager_wait_connected(30000))
    {
        ESP_LOGE(TAG, "WiFi connection timeout");
        return;
    }
    ESP_LOGI(TAG, "WiFi connected");

    // 5. Initialize and start MQTT
    ESP_ERROR_CHECK(mqtt_manager_init());
    ESP_ERROR_CHECK(mqtt_manager_start());
    ESP_LOGI(TAG, "MQTT client started");

    // Stack needs to be large due to JSON serialization of batches
    xTaskCreate(mqtt_task, "mqtt", 16384, NULL, MQTT_TASK_PRIORITY, NULL);

    xTaskCreate(processing_task, "process", 4096, NULL, PROCESSING_TASK_PRIORITY, NULL);
    xTaskCreate(sensor_task, "sensor", 4096, NULL, SENSOR_TASK_PRIORITY, NULL);

    ESP_LOGI(TAG, "Tasks created, system running");

    // TEMPORARY: Send mock data for testing
    // send_mock_data();
}