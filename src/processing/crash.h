#ifndef CRASH_H
#define CRASH_H

#include <stdbool.h>
#include "message_types.h"

// Default crash threshold in g-force (can be overridden via server command)
#define DEFAULT_CRASH_THRESHOLD_G 8.0f

// Initialize crash detection (call once at startup)
void crash_detection_init(void);

// Set crash threshold (called when server sends new threshold)
void crash_set_threshold(float threshold_g);

// Get current crash threshold
float crash_get_threshold(void);

// Detect if a crash has occurred based on acceleration magnitude
bool detect_crash(const sensor_reading_t *data);

// Handle a detected crash event
void handle_crash(const sensor_reading_t *data);

#endif // CRASH_H
