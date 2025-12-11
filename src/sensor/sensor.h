#ifndef SENSOR_H
#define SENSOR_H

#include "message_types.h"

// Uncomment to use mock sensor data instead of real IMU
// #define MOCK_SENSOR_DATA

// Sensor sampling interval in milliseconds (derived from SAMPLE_RATE_HZ)
#define SENSOR_INTERVAL_MS (1000 / SAMPLE_RATE_HZ)

// Read accelerometer data from IMU
sensor_reading_t read_imu(void);

// FreeRTOS task for reading sensor data
// Priority 5 - highest priority task
void sensor_task(void *pvParameters);

#endif // SENSOR_H
