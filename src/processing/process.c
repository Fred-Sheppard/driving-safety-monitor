#include "message_types.h"
#include "queue/ring_buffer.h"
#include "queue/ring_buffer_utils.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "trace/trace.h"
#include "watchdog/watchdog.h"
#include "detector.h"

static const char *TAG = "process";

static sensor_batch_t current_batch;
static uint16_t batch_index = 0;

static void batch_telemetry_reading(const sensor_reading_t *data);
static void handle_mqtt_command(const mqtt_command_t *cmd);
static void send_status_response(void);

void processing_task(void *pvParameters)
{
    (void)pvParameters;
    sensor_reading_t sensor_data;
    mqtt_command_t mqtt_cmd;

    current_batch.sample_rate_hz = IMU_SAMPLE_RATE_HZ;
    current_batch.sample_count = 0;
    batch_index = 0;

    detectors_init();

    ESP_LOGI(TAG, "Processing task started");
    watchdog_register_task();

    while (1)
    {
        TRACE_TASK_RUN(TAG);
        watchdog_feed();

        while (ring_buffer_pop_front(mqtt_command_queue, &mqtt_cmd))
        {
            handle_mqtt_command(&mqtt_cmd);
        }

        if (ring_buffer_pop_front(sensor_rb, &sensor_data))
        {
            detectors_check_all(&sensor_data);
            batch_telemetry_reading(&sensor_data);
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

static void batch_telemetry_reading(const sensor_reading_t *data)
{
    if (batch_index == 0)
    {
        current_batch.batch_start_timestamp = xTaskGetTickCount();
    }

    current_batch.samples[batch_index] = *data;
    batch_index++;

    if (batch_index >= LOG_BATCH_SIZE)
    {
        current_batch.sample_count = batch_index;

        if (!ring_buffer_push_back_with_full_log(batch_rb, &current_batch,
                                                 "batch_rb full, overwrote oldest batch"))
        {
            ESP_LOGW(TAG, "batch_rb: failed to push telemetry batch");
        }

        batch_index = 0;
    }
}

static void send_status_response(void)
{
    mqtt_command_t response = {
        .type = MQTT_RESP_STATUS,
        .data.status = {
            .crash = detector_get_threshold(DETECTOR_CRASH),
            .braking = detector_get_threshold(DETECTOR_HARSH_BRAKING),
            .accel = detector_get_threshold(DETECTOR_HARSH_ACCEL),
            .cornering = detector_get_threshold(DETECTOR_HARSH_CORNERING)}};


    if (!ring_buffer_push_back_with_full_log(mqtt_response_queue, &response,
                                             "Response queue full, overwrote oldest response"))
    {
        ESP_LOGW(TAG, "Failed to queue status response");
    }
}

static void handle_mqtt_command(const mqtt_command_t *cmd)
{
    switch (cmd->type)
    {
    case MQTT_CMD_SET_THRESHOLD:
    {
        detector_type_t detector;
        switch (cmd->data.set_threshold.threshold)
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
            ESP_LOGW(TAG, "Unknown threshold type: %d", cmd->data.set_threshold.threshold);
            return;
        }

        detector_set_threshold(detector, cmd->data.set_threshold.value);
        ESP_LOGI(TAG, "Set %s threshold to %.1f G",
                 detector_get_name(detector), cmd->data.set_threshold.value);

        send_status_response();
        break;
    }

    case MQTT_CMD_GET_STATUS:
        ESP_LOGI(TAG, "Status requested");
        send_status_response();
        break;

    default:
        ESP_LOGW(TAG, "Unknown MQTT command type: %d", cmd->type);
        break;
    }
}
