#pragma once
#include "message_types.h"
#include "esp_err.h"

esp_err_t sensor_i2c_init(void);
sensor_reading_t read_imu(void);
void sensor_task(void *pvParameters);
