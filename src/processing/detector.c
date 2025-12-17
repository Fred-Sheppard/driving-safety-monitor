#include "detector.h"
#include "detector_defs.h"
#include "message_types.h"
#include "queue/ring_buffer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "detector";

static mqtt_message_t build_crash_message(float magnitude)
{
    return (mqtt_message_t){
        .type = MSG_CRASH,
        .data.crash = {
            .timestamp = xTaskGetTickCount(),
            .accel_magnitude = magnitude,
        },
    };
}

static mqtt_message_t build_warning_message(warning_event_t event, const sensor_reading_t *data)
{
    return (mqtt_message_t){
        .type = MSG_WARNING,
        .data.warning = {
            .event = event,
            .timestamp = xTaskGetTickCount(),
            .accel_x = data->x,
            .accel_y = data->y,
        },
    };
}

void detectors_init(void)
{
    for (int i = 0; i < DETECTOR_COUNT; i++) {
        detectors[i].threshold = detectors[i].default_threshold;
    }
    ESP_LOGI(TAG, "Detectors initialized");
}

void detector_set_threshold(detector_type_t type, float threshold_g)
{
    if (type >= DETECTOR_COUNT) return;
    detectors[type].threshold = threshold_g;
    ESP_LOGI(TAG, "%s threshold set to %.2f g", detectors[type].name, threshold_g);
}

float detector_get_threshold(detector_type_t type)
{
    if (type >= DETECTOR_COUNT) return 0.0f;
    return detectors[type].threshold;
}

const char *detector_get_name(detector_type_t type)
{
    if (type >= DETECTOR_COUNT) return "unknown";
    return detectors[type].name;
}

static void handle_detection(detector_config_t *det, const sensor_reading_t *data)
{
    float value = det->get_value(data);

    mqtt_message_t msg = det->is_crash
        ? build_crash_message(value)
        : build_warning_message(det->warning_event, data);

    bool was_full = false;
    bool success = false;
    if (det->is_crash) {
        // High priority - send immediately
        success = ring_buffer_push_front(mqtt_rb, &msg, &was_full);
    } else {
        success = ring_buffer_push_back(mqtt_rb, &msg, &was_full);
    }

    if (!success) {
        ESP_LOGW(TAG, "mqtt_rb: failed to push %s alert", det->name);
    } else {
        if (was_full)
            ESP_LOGW(TAG, "mqtt_rb full, overwrote %s alert", det->is_crash ? "newest" : "oldest");
        ESP_LOGI(TAG, "%s detected! Value: %.2f g", det->name, value);
    }

    if (det->on_trigger) {
        det->on_trigger(det->display_name);
    }
}

void detectors_check_all(const sensor_reading_t *data)
{
    for (int i = 0; i < DETECTOR_COUNT; i++) {
        detector_config_t *det = &detectors[i];
        if (det->check(data, det->threshold)) {
            handle_detection(det, data);
        }
    }
}
