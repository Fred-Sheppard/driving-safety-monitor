#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "message_types.h"
#include "wifi/wifi_manager.h"
#include "mqtt/mqtt_manager.h"

static const char *TAG = "main";

// Queue handles (declared extern in message_types.h)
QueueHandle_t sensor_queue = NULL;
QueueHandle_t batch_queue = NULL;
QueueHandle_t mqtt_queue = NULL;
QueueHandle_t command_queue = NULL;

void app_main(void) {
    ESP_LOGI(TAG, "Driving Safety Monitor starting...");

    // 1. Initialize NVS (required for WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Create queues
    mqtt_queue = xQueueCreate(MQTT_QUEUE_SIZE, sizeof(mqtt_message_t));
    batch_queue = xQueueCreate(BATCH_QUEUE_SIZE, sizeof(sensor_batch_t));
    sensor_queue = xQueueCreate(SENSOR_QUEUE_SIZE, sizeof(sensor_reading_t));
    command_queue = xQueueCreate(COMMAND_QUEUE_SIZE, sizeof(uint8_t));

    if (mqtt_queue == NULL || batch_queue == NULL ||
        sensor_queue == NULL || command_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create queues");
        return;
    }
    ESP_LOGI(TAG, "Queues created successfully");

    // 3. Initialize WiFi
    ESP_ERROR_CHECK(wifi_manager_init());

    // 4. Wait for WiFi connection (30 second timeout)
    if (!wifi_manager_wait_connected(30000)) {
        ESP_LOGE(TAG, "WiFi connection timeout");
        return;
    }
    ESP_LOGI(TAG, "WiFi connected");

    // 5. Initialize and start MQTT
    ESP_ERROR_CHECK(mqtt_manager_init());
    ESP_ERROR_CHECK(mqtt_manager_start());
    ESP_LOGI(TAG, "MQTT client started");

    // 6. Create mqtt_task with priority 2 (lowest of the three tasks)
    // Stack needs to be large due to JSON serialization of batches
    xTaskCreate(
        mqtt_task,
        "mqtt",
        16384,
        NULL,
        2,
        NULL
    );

    // TODO: Create sensor_task and processing_task
    // xTaskCreate(processing_task, "process", 4096, NULL, 4, NULL);
    // xTaskCreate(sensor_task, "sensor", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "Tasks created, system running");

    // ===== TEMPORARY: Send mock data for testing =====
    vTaskDelay(pdMS_TO_TICKS(3000));  // Wait for MQTT to connect

    // Send a mock warning alert
    ESP_LOGI(TAG, "Sending mock warning alert...");
    mqtt_message_t warning_msg = {
        .type = MSG_WARNING,
        .data.warning = {
            .event = WARNING_HARSH_BRAKING,
            .timestamp = xTaskGetTickCount(),
            .accel_y = -4.5f
        }
    };
    xQueueSend(mqtt_queue, &warning_msg, 0);

    vTaskDelay(pdMS_TO_TICKS(1000));

    // Send a mock crash alert
    ESP_LOGI(TAG, "Sending mock crash alert...");
    mqtt_message_t crash_msg = {
        .type = MSG_CRASH,
        .data.crash = {
            .timestamp = xTaskGetTickCount(),
            .accel_magnitude = 15.2f
        }
    };
    xQueueSend(mqtt_queue, &crash_msg, 0);

    vTaskDelay(pdMS_TO_TICKS(1000));

    // Send a mock telemetry batch (small - 10 samples for testing)
    ESP_LOGI(TAG, "Sending mock telemetry batch...");
    static sensor_batch_t mock_batch;  // static to avoid stack overflow
    mock_batch.batch_start_timestamp = xTaskGetTickCount();
    mock_batch.sample_rate_hz = SAMPLE_RATE_HZ;
    mock_batch.sample_count = 10;  // Only 10 samples for testing

    for (int i = 0; i < 10; i++) {
        mock_batch.samples[i].x = 0.01f * i;
        mock_batch.samples[i].y = -9.81f + (0.1f * i);
        mock_batch.samples[i].z = 0.02f * i;
    }
    xQueueSend(batch_queue, &mock_batch, 0);

    ESP_LOGI(TAG, "Mock data sent! Check the bridge logs.");
    // ===== END TEMPORARY =====
}
