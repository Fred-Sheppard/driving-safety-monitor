#include "message_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "trace/trace.h"
#include "detector.h"
#include "queue/bidir_queue.h"
#include "queue/policy_based_queue.h"

static const char *TAG = "process";

// Batch accumulator
static sensor_batch_t current_batch;
static uint16_t batch_index = 0;

// Forward declarations
static void batch_telemetry_reading(const sensor_reading_t *data);
static void handle_bidir_command(const bidir_message_t *msg);
static void send_status_response(void);

void processing_task(void *pvParameters)
{
    (void)pvParameters;
    sensor_reading_t sensor_data;
    bidir_message_t bidir_msg;

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

        // Process all pending inbound commands (non-blocking)
        while (bidir_queue_pop(BIDIR_INBOUND, &bidir_msg))
        {
            handle_bidir_command(&bidir_msg);
        }

        // Wait for sensor data (with timeout)
        if (xQueueReceive(sensor_queue, &sensor_data, pdMS_TO_TICKS(100)) == pdTRUE)
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
        if (!send_to_queue(batch_queue, &current_batch, sizeof(current_batch)))
        {
            ESP_LOGW(TAG, "batch_queue: failed to queue telemetry batch");
        }

        // Reset for next batch
        batch_index = 0;
    }
}

static void send_status_response(void)
{
    bidir_message_t response = {
        .direction = BIDIR_OUTBOUND,
        .type = BIDIR_RESP_STATUS,
        .data.status = {
            .crash = detector_get_threshold(DETECTOR_CRASH),
            .braking = detector_get_threshold(DETECTOR_HARSH_BRAKING),
            .accel = detector_get_threshold(DETECTOR_HARSH_ACCEL),
            .cornering = detector_get_threshold(DETECTOR_HARSH_CORNERING)}};

    if (!bidir_queue_push(&response))
    {
        ESP_LOGW(TAG, "Failed to queue status response");
    }
}

static void handle_bidir_command(const bidir_message_t *msg)
{
    switch (msg->type)
    {
    case BIDIR_CMD_SET_THRESHOLD:
    {
        detector_type_t detector;
        switch (msg->data.set_threshold.threshold)
        {
        case THRESHOLD_CRASH:
            detector = DETECTOR_CRASH;
            break;
        case THRESHOLD_BRAKING:
            detector = DETECTOR_HARSH_BRAKING;
            break;
        case THRESHOLD_ACCEL:
            detector = DETECTOR_HARSH_ACCEL;
            break;
        case THRESHOLD_CORNERING:
            detector = DETECTOR_HARSH_CORNERING;
            break;
        default:
            ESP_LOGW(TAG, "Unknown threshold type: %d", msg->data.set_threshold.threshold);
            return;
        }

        detector_set_threshold(detector, msg->data.set_threshold.value);
        ESP_LOGI(TAG, "Set %s threshold to %.1f G",
                 detector_get_name(detector), msg->data.set_threshold.value);

        // Send updated status back to dashboard
        send_status_response();
        break;
    }

    case BIDIR_CMD_GET_STATUS:
        ESP_LOGI(TAG, "Status requested");
        send_status_response();
        break;

    default:
        ESP_LOGW(TAG, "Unknown bidir command type: %d", msg->type);
        break;
    }
}
