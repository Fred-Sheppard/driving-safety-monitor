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
static void handle_command(const command_t *cmd);

void processing_task(void *pvParameters)
{
    (void)pvParameters;
    sensor_reading_t sensor_data;
    command_t cmd;

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
        while (xQueueReceive(command_queue, &cmd, 0) == pdTRUE)
        {
            handle_command(&cmd);
        }

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

static void handle_command(const command_t *cmd)
{
    detector_type_t detector;

    switch (cmd->type)
    {
    case CMD_SET_CRASH_THRESHOLD:
        detector = DETECTOR_CRASH;
        break;
    case CMD_SET_BRAKING_THRESHOLD:
        detector = DETECTOR_HARSH_BRAKING;
        break;
    case CMD_SET_ACCEL_THRESHOLD:
        detector = DETECTOR_HARSH_ACCEL;
        break;
    case CMD_SET_CORNERING_THRESHOLD:
        detector = DETECTOR_HARSH_CORNERING;
        break;
    default:
        ESP_LOGW(TAG, "Unknown command type: %d", cmd->type);
        return;
    }

    detector_set_threshold(detector, cmd->value);
    ESP_LOGI(TAG, "Set %s threshold to %.1f G",
             detector_get_name(detector), cmd->value);
}
