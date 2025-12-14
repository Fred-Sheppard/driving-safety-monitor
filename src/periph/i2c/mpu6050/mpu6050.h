#pragma once
#include "esp_err.h"

#define MPU6050_ADDR            0x68
#define MPU6050_PWR_MGMT_1      0x6B
#define MPU6050_ACCEL_XOUT_H    0x3B
#define MPU6050_ACCEL_CONFIG    0x1C
#define MPU6050_ACCEL_SCALE     16384.0f

esp_err_t mpu6050_init(void);
void mpu6050_read_accel(float *ax, float *ay, float *az);
