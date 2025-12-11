#ifndef MESSAGE_TYPES_H
#define MESSAGE_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// Queue sizes from design docs
#define SENSOR_QUEUE_SIZE   10
#define MQTT_QUEUE_SIZE     20
#define COMMAND_QUEUE_SIZE  5
#define BATCH_QUEUE_SIZE    3

// Batch configuration
#define BATCH_SIZE          500
#define SAMPLE_RATE_HZ      100

// MQTT Topics
#define MQTT_TOPIC_ALERTS     "driving/alerts"
#define MQTT_TOPIC_TELEMETRY  "driving/telemetry"

// Message type enumeration
typedef enum {
    MSG_WARNING,
    MSG_CRASH
} message_type_t;

// Warning event types
typedef enum {
    WARNING_HARSH_BRAKING,
    WARNING_HARSH_ACCEL
} warning_event_t;

// Warning data structure
typedef struct {
    warning_event_t event;
    uint32_t timestamp;
    float accel_y;
} warning_data_t;

// Crash data structure
typedef struct {
    uint32_t timestamp;
    float accel_magnitude;
} crash_data_t;

// MQTT message union for alerts (sent via mqtt_queue)
typedef struct {
    message_type_t type;
    union {
        warning_data_t warning;
        crash_data_t crash;
    } data;
} mqtt_message_t;

// Sensor reading (individual sample)
typedef struct {
    float x;
    float y;
    float z;
} sensor_reading_t;

// Batch telemetry structure (sent via batch_queue)
typedef struct {
    uint32_t batch_start_timestamp;
    uint16_t sample_rate_hz;
    uint16_t sample_count;
    sensor_reading_t samples[BATCH_SIZE];
} sensor_batch_t;

// External queue handles (defined in main.c)
extern QueueHandle_t sensor_queue;
extern QueueHandle_t batch_queue;
extern QueueHandle_t mqtt_queue;
extern QueueHandle_t command_queue;

#endif // MESSAGE_TYPES_H
