#include "message_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "trace/trace.h"
#include "detector.h"

static const char *TAG = "process";

// Batch accumulator
static sensor_batch_t current_batch;
static uint16_t batch_index = 0;

// Forward declarations
static void batch_telemetry_reading(const sensor_reading_t *data);

void processing_task(void *pvParameters)
{
    (void)pvParameters;
    sensor_reading_t sensor_data;

    // Initialize batch
    current_batch.sample_rate_hz = IMU_SAMPLE_RATE_HZ;
    current_batch.sample_count = 0;
    batch_index = 0;

    // Initialize detection modules
    detectors_init();

    ESP_LOGI(TAG, "Processing task started");

    while (1)
    {
        TRACE_TASK_RUN(TAG);
        // Process all pending commands (non-blocking)
        //
        // How to read this:
        // Because we pass in 0, FreeRTOS will not wait at all if there is nothing
        // in the queue. It will just skip the loop. The while loop handles each
        // command in the queue, which is not blocking
        //
        // It also makes sense to handle commands more readily than sensor data,
        // since setting the threshold will affect how we react to the sensor data.

        // while (xQueueReceive(command_queue, &cmd, 0) == pdTRUE)
        // {
        //     handle_command(&cmd);
        // }

        // Wait for sensor data (with timeout)
        if (xQueueReceive(sensor_queue, &sensor_data, pdMS_TO_TICKS(100)) ==
            pdTRUE)
        {
            detectors_check_all(&sensor_data);
            batch_telemetry_reading(&sensor_data);
        }
    }
}

static void batch_telemetry_reading(const sensor_reading_t *data)
{
    // Start a new batch if this is the first sample
    if (batch_index == 0)
    {
        current_batch.batch_start_timestamp = xTaskGetTickCount();
    }

    // Add sample to batch
    current_batch.samples[batch_index] = *data;
    batch_index++;

    // Send batch when full
    if (batch_index >= LOG_BATCH_SIZE)
    {
        current_batch.sample_count = batch_index;

        // Send to batch_queue (non-blocking)
        if (xQueueSend(batch_queue, &current_batch, 0) != pdTRUE)
        {
            ESP_LOGW(TAG, "batch_queue: failed to queue telemetry batch. Queue full");
        }

        // Reset for next batch
        batch_index = 0;
    }
}
