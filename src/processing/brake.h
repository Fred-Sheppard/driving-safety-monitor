#ifndef BRAKE_H
#define BRAKE_H

#include <stdbool.h>
#include "message_types.h"

// Initialize brake detection (call once at startup)
void brake_detection_init(void);

// Set harsh braking threshold (called when server sends new threshold)
void brake_set_threshold(float threshold_g);

// Get current harsh braking threshold
float brake_get_threshold(void);

// Detect if harsh braking has occurred
bool detect_harsh_braking(const sensor_reading_t *data);

// Handle a detected harsh braking event
void handle_harsh_braking(const sensor_reading_t *data);

#endif // BRAKE_H
