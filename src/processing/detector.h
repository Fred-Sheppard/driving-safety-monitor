#ifndef DETECTOR_H
#define DETECTOR_H

#include <stdbool.h>
#include "message_types.h"

typedef enum {
    DETECTOR_CRASH,
    DETECTOR_HARSH_BRAKING,
    DETECTOR_HARSH_ACCEL,
    DETECTOR_HARSH_CORNERING,
    DETECTOR_COUNT
} detector_type_t;

void detectors_init(void);
void detector_set_threshold(detector_type_t type, float threshold_g);
float detector_get_threshold(detector_type_t type);
void detectors_check_all(const sensor_reading_t *data);
const char *detector_get_name(detector_type_t type);

#endif // DETECTOR_H
