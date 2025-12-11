#ifndef ACCELERATE_H
#define ACCELERATE_H

#include <stdbool.h>
#include "message_types.h"

// Initialize acceleration detection (call once at startup)
void accel_detection_init(void);

// Set harsh acceleration threshold (called when server sends new threshold)
void accel_set_threshold(float threshold_g);

// Get current harsh acceleration threshold
float accel_get_threshold(void);

// Detect if harsh acceleration has occurred
bool detect_harsh_acceleration(const sensor_reading_t *data);

// Handle a detected harsh acceleration event
void handle_harsh_acceleration(const sensor_reading_t *data);

#endif // ACCELERATE_H
