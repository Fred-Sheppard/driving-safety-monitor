#ifndef SENSOR_H
#define SENSOR_H

#include "message_types.h"

// Read accelerometer data from IMU
sensor_reading_t read_imu(void);

// FreeRTOS task for reading sensor data
// Priority 5 - highest priority task
void sensor_task(void *pvParameters);

#endif // SENSOR_H
