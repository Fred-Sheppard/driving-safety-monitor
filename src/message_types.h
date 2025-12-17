#ifndef MESSAGE_TYPES_H
#define MESSAGE_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

typedef struct ring_buffer ring_buffer_t;

typedef enum {
    MSG_WARNING,
    MSG_CRASH
} message_type_t;

typedef enum {
    WARNING_HARSH_BRAKING,
    WARNING_HARSH_ACCEL,
    WARNING_HARSH_CORNERING
} warning_event_t;

static inline const char *warning_event_to_string(warning_event_t event) {
    switch (event) {
        case WARNING_HARSH_BRAKING:   return "harsh_braking";
        case WARNING_HARSH_ACCEL:     return "harsh_accel";
        case WARNING_HARSH_CORNERING: return "harsh_cornering";
        default:                      return "unknown";
    }
}

typedef struct {
    warning_event_t event;
    uint32_t timestamp;
    float accel_x; // Lateral (cornering)
    float accel_y; // Longitudinal (braking/acceleration)
} warning_data_t;

typedef struct {
    uint32_t timestamp;
    float accel_magnitude;
} crash_data_t;

typedef struct {
    message_type_t type;
    union {
        warning_data_t warning;
        crash_data_t crash;
    } data;
} mqtt_message_t;

typedef struct {
    float x;
    float y;
    float z;
} sensor_reading_t;

typedef struct {
    uint32_t batch_start_timestamp;
    uint16_t sample_rate_hz;
    uint16_t sample_count;
    sensor_reading_t samples[LOG_BATCH_SIZE];
} sensor_batch_t;

typedef enum {
    THRESHOLD_CRASH,
    THRESHOLD_BRAKING,
    THRESHOLD_ACCEL,
    THRESHOLD_CORNERING
} threshold_type_t;

typedef enum {
    BIDIR_INBOUND,   // Command from dashboard
    BIDIR_OUTBOUND   // Response/status to dashboard
} bidir_direction_t;

typedef enum {
    BIDIR_CMD_SET_THRESHOLD,  // Inbound: set a threshold
    BIDIR_CMD_GET_STATUS,     // Inbound: request current status
    BIDIR_RESP_STATUS         // Outbound: current thresholds
} bidir_msg_type_t;

typedef struct {
    float crash;
    float braking;
    float accel;
    float cornering;
} threshold_status_t;

typedef struct {
    bidir_direction_t direction;
    bidir_msg_type_t type;
    union {
        struct {
            threshold_type_t threshold;
            float value;
        } set_threshold;
        threshold_status_t status;
    } data;
} bidir_message_t;

extern ring_buffer_t *sensor_rb;
extern ring_buffer_t *batch_rb;
extern ring_buffer_t *mqtt_rb;

#endif // MESSAGE_TYPES_H
