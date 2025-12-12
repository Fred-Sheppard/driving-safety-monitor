#include "sensor.h"
#include "config.h"
#include "message_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include <math.h>

static const char *TAG = "sensor";

// I2C Configuration
#define I2C_MASTER_NUM      I2C_NUM_0
#define I2C_MASTER_SDA      26
#define I2C_MASTER_SCL      25
#define I2C_MASTER_FREQ_HZ  400000
#define I2C_TIMEOUT_MS      1000

// MPU6050 Configuration
#define MPU6050_ADDR            0x68
#define MPU6050_PWR_MGMT_1      0x6B
#define MPU6050_ACCEL_XOUT_H    0x3B
#define MPU6050_ACCEL_CONFIG    0x1C
#define MPU6050_ACCEL_SCALE     16384.0f  // For ±2g range

// Mock sensor configuration
#ifdef MOCK_SENSOR_DATA
#define MOCK_LATERAL_AMPLITUDE   0.1f
#define MOCK_FORWARD_AMPLITUDE   0.2f
#define MOCK_GRAVITY_BASE        9.81f
#define MOCK_GRAVITY_VARIATION   0.05f
#define MOCK_BRAKE_EVENT_TICKS   500
#define MOCK_BRAKE_FORCE        -2.0f
#endif

// ============================================================================
// Private I2C Helper Functions
// ============================================================================

static esp_err_t mpu6050_write_byte(uint8_t reg, uint8_t data) {
    uint8_t write_buf[2] = {reg, data};
    return i2c_master_write_to_device(
        I2C_MASTER_NUM, 
        MPU6050_ADDR, 
        write_buf, 
        sizeof(write_buf),
        pdMS_TO_TICKS(I2C_TIMEOUT_MS)
    );
}

static esp_err_t mpu6050_read_bytes(uint8_t reg, uint8_t *data, size_t len) {
    return i2c_master_write_read_device(
        I2C_MASTER_NUM,
        MPU6050_ADDR,
        &reg,
        1,
        data,
        len,
        pdMS_TO_TICKS(I2C_TIMEOUT_MS)
    );
}

// ============================================================================
// MPU6050 Sensor Functions
// ============================================================================

static void mpu6050_read_accel(float *ax, float *ay, float *az) {
    uint8_t data[6];
    esp_err_t ret = mpu6050_read_bytes(MPU6050_ACCEL_XOUT_H, data, sizeof(data));
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read accelerometer data: %s", esp_err_to_name(ret));
        *ax = *ay = *az = 0.0f;
        return;
    }
    
    // Combine high and low bytes (big-endian)
    int16_t raw_x = (int16_t)((data[0] << 8) | data[1]);
    int16_t raw_y = (int16_t)((data[2] << 8) | data[3]);
    int16_t raw_z = (int16_t)((data[4] << 8) | data[5]);
    
    // Convert to G-force (±2g range)
    *ax = raw_x / MPU6050_ACCEL_SCALE;
    *ay = raw_y / MPU6050_ACCEL_SCALE;
    *az = raw_z / MPU6050_ACCEL_SCALE;
}

#ifdef MOCK_SENSOR_DATA
static sensor_reading_t read_mock_imu(void) {
    static uint32_t tick = 0;
    tick++;
    
    sensor_reading_t reading = {
        .x = MOCK_LATERAL_AMPLITUDE * sinf(tick * 0.05f),
        .y = MOCK_FORWARD_AMPLITUDE * sinf(tick * 0.02f),
        .z = MOCK_GRAVITY_BASE + MOCK_GRAVITY_VARIATION * sinf(tick * 0.1f)
    };
    
    // Simulate braking event periodically
    if (tick % MOCK_BRAKE_EVENT_TICKS == 0) {
        reading.y = MOCK_BRAKE_FORCE;
        ESP_LOGI(TAG, "Mock brake event");
    }
    
    return reading;
}
#endif

// ============================================================================
// Public API
// ============================================================================

sensor_reading_t read_imu(void) {
    float ax, ay, az;
    mpu6050_read_accel(&ax, &ay, &az);
    
    sensor_reading_t reading = {
        .x = ax,
        .y = ay,
        .z = az
    };
    
    return reading;
}

esp_err_t sensor_i2c_init(void) {
    esp_err_t ret;
    
    // Check if I2C driver is already installed (by display init)
    // If not installed, configure and install it
    ret = i2c_driver_install(I2C_MASTER_NUM, I2C_MODE_MASTER, 0, 0, 0);
    
    if (ret == ESP_ERR_INVALID_STATE) {
        // I2C already initialized (likely by display), this is fine
        ESP_LOGI(TAG, "I2C already initialized, using existing driver");
    } else if (ret == ESP_OK) {
        // We installed it, need to configure
        i2c_config_t conf = {
            .mode = I2C_MODE_MASTER,
            .sda_io_num = I2C_MASTER_SDA,
            .scl_io_num = I2C_MASTER_SCL,
            .sda_pullup_en = GPIO_PULLUP_ENABLE,
            .scl_pullup_en = GPIO_PULLUP_ENABLE,
            .master.clk_speed = I2C_MASTER_FREQ_HZ,
            .clk_flags = 0
        };
        
        ret = i2c_param_config(I2C_MASTER_NUM, &conf);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "I2C param config failed: %s", esp_err_to_name(ret));
            return ret;
        }
        
        ESP_LOGI(TAG, "I2C initialized on SDA=%d, SCL=%d", I2C_MASTER_SDA, I2C_MASTER_SCL);
    } else {
        ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Initialize MPU6050
    vTaskDelay(pdMS_TO_TICKS(100));  // Allow sensor to power up
    
    ret = mpu6050_write_byte(MPU6050_PWR_MGMT_1, 0x00);  // Wake up
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to wake MPU6050: %s", esp_err_to_name(ret));
        return ret;
    }
    
    vTaskDelay(pdMS_TO_TICKS(10));
    
    ret = mpu6050_write_byte(MPU6050_ACCEL_CONFIG, 0x00);  // ±2g range
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure MPU6050: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "MPU6050 initialized successfully");
    return ESP_OK;
}

void sensor_task(void *pvParameters) {
    (void)pvParameters;
    sensor_reading_t reading;
    
    ESP_LOGI(TAG, "Sensor task started (interval: %d ms)", SENSOR_INTERVAL_MS);
    
#ifndef MOCK_SENSOR_DATA
    // Initialize MPU6050 only (I2C should already be initialized in main)
    vTaskDelay(pdMS_TO_TICKS(100));  // Allow sensor to power up
    
    esp_err_t ret = mpu6050_write_byte(MPU6050_PWR_MGMT_1, 0x00);  // Wake up
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to wake MPU6050: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "Sensor initialization failed, task terminating");
        vTaskDelete(NULL);
        return;
    }
    
    vTaskDelay(pdMS_TO_TICKS(10));
    
    ret = mpu6050_write_byte(MPU6050_ACCEL_CONFIG, 0x00);  // ±2g range
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure MPU6050: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "Sensor initialization failed, task terminating");
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAG, "MPU6050 initialized successfully");
#else
    ESP_LOGW(TAG, "Using MOCK sensor data");
#endif
    
    while (1) {
#ifdef MOCK_SENSOR_DATA
        reading = read_mock_imu();
#else
        reading = read_imu();
#endif
        
        // Send to queue with non-blocking send
        if (xQueueSend(sensor_queue, &reading, 0) != pdTRUE) {
            ESP_LOGW(TAG, "Sensor queue full, dropping sample");
        }
        
        vTaskDelay(pdMS_TO_TICKS(SENSOR_INTERVAL_MS));
    }
}