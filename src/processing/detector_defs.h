#ifndef DETECTOR_DEFS_H
#define DETECTOR_DEFS_H

#include <stdbool.h>
#include "detector.h"
#include "message_types.h"

typedef struct {
    const char *name;
    float default_threshold;
    float threshold;
    bool is_crash;
    warning_event_t warning_event;
    bool (*check)(const sensor_reading_t *data, float threshold);
    float (*get_value)(const sensor_reading_t *data);
    void (*on_trigger)(void);
} detector_config_t;

extern detector_config_t detectors[DETECTOR_COUNT];

#endif // DETECTOR_DEFS_H
