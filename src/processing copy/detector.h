#ifndef DETECTOR_H
#define DETECTOR_H

#include <stdbool.h>
#include "message_types.h"

// Detector type enumeration
typedef enum {
    DETECTOR_CRASH,
    DETECTOR_HARSH_BRAKING,
    DETECTOR_HARSH_ACCEL,
    DETECTOR_HARSH_CORNERING,
    DETECTOR_COUNT
} detector_type_t;

// Initialize all detectors
void detectors_init(void);

// Set threshold for a specific detector
void detector_set_threshold(detector_type_t type, float threshold_g);

// Get threshold for a specific detector
float detector_get_threshold(detector_type_t type);

// Run all detectors on sensor data, queue alerts for any triggers
void detectors_check_all(const sensor_reading_t *data);

// Get detector name (for logging)
const char *detector_get_name(detector_type_t type);

#endif // DETECTOR_H
