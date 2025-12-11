#ifndef CORNER_H
#define CORNER_H

#include <stdbool.h>
#include "message_types.h"

// Initialize cornering detection (call once at startup)
void corner_detection_init(void);

// Set harsh cornering threshold (called when server sends new threshold)
void corner_set_threshold(float threshold_g);

// Get current harsh cornering threshold
float corner_get_threshold(void);

// Detect if harsh cornering has occurred
bool detect_harsh_cornering(const sensor_reading_t *data);

// Handle a detected harsh cornering event
void handle_harsh_cornering(const sensor_reading_t *data);

#endif // CORNER_H
