/**
 * @file sensor.h
 * @brief MPU6050 accelerometer sensor interface
 * 
 * Provides functions to initialize I2C, read accelerometer data,
 * and run a continuous sensor monitoring task.
 */

#ifndef SENSOR_H
#define SENSOR_H

#include "message_types.h"
#include "esp_err.h"

/**
 * @brief Initialize the I2C bus for sensor communication
 * 
 * Configures I2C_NUM_0 with:
 * - SDA: GPIO 26
 * - SCL: GPIO 25
 * - Clock: 400kHz
 * - Internal pull-ups enabled
 * 
 * MUST be called from app_main() BEFORE creating sensor_task.
 * 
 * @return ESP_OK on success
 * @return ESP_FAIL if I2C configuration or installation fails
 * 
 * @note Call this only once during system initialization
 * 
 * Example:
 * @code
 * void app_main() {
 *     if (sensor_i2c_init() != ESP_OK) {
 *         ESP_LOGE("main", "I2C init failed");
 *         return;
 *     }
 *     xTaskCreate(sensor_task, "sensor", 4096, NULL, 5, NULL);
 * }
 * @endcode
 */
esp_err_t sensor_i2c_init(void);

/**
 * @brief Read current accelerometer data from MPU6050
 * 
 * Reads 3-axis acceleration data in G-force units (±2g range).
 * 
 * @return sensor_reading_t Structure containing:
 *         - x: X-axis acceleration (G)
 *         - y: Y-axis acceleration (G)
 *         - z: Z-axis acceleration (G)
 * 
 * @note If I2C read fails, returns zeros and logs error
 * @note Typical values: {0.0, 0.0, 9.81} when stationary and level
 */
sensor_reading_t read_imu(void);

/**
 * @brief FreeRTOS task for continuous sensor reading
 * 
 * This task:
 * 1. Initializes the MPU6050 sensor (wakes it up, configures range)
 * 2. Continuously reads accelerometer data at SENSOR_INTERVAL_MS rate
 * 3. Sends readings to sensor_queue
 * 
 * If MOCK_SENSOR_DATA is defined at compile time, generates simulated
 * data instead of reading from hardware.
 * 
 * @param pvParameters Unused FreeRTOS task parameter (pass NULL)
 * 
 * @note CRITICAL: Call sensor_i2c_init() BEFORE creating this task!
 * @note Task will self-terminate if MPU6050 initialization fails
 * @note Reads from I2C_NUM_0 at address 0x68
 * @note Uses sensor_queue (must be created before task starts)
 * 
 * Example:
 * @code
 * sensor_i2c_init();
 * xTaskCreate(sensor_task, "sensor", 4096, NULL, 5, NULL);
 * @endcode
 */
void sensor_task(void *pvParameters);

#endif // SENSOR_H