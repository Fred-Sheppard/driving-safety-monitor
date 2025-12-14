#include "mpu6050.h"
#include "../i2c_bus.h"
#include "driver/i2c.h"
#include "esp_log.h"

static const char *TAG = "mpu6050";

esp_err_t mpu6050_init(void) {
    vTaskDelay(pdMS_TO_TICKS(100));

    esp_err_t ret;

    ret = i2c_bus_write_byte(MPU6050_ADDR, MPU6050_PWR_MGMT_1, 0x00);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to wake MPU6050: %s", esp_err_to_name(ret));
        return ret;
    }

    vTaskDelay(pdMS_TO_TICKS(10));

    ret = i2c_bus_write_byte(MPU6050_ADDR, MPU6050_ACCEL_CONFIG, 0x00);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure MPU6050: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "MPU6050 ready");
    return ESP_OK;
}

void mpu6050_read_accel(float *ax, float *ay, float *az) {
    uint8_t data[6];
    esp_err_t ret = i2c_bus_read_bytes(MPU6050_ADDR, MPU6050_ACCEL_XOUT_H, data, 6);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Accel read failed");
        *ax = *ay = *az = 0.0f;
        return;
    }

    int16_t raw_x = (data[0] << 8) | data[1];
    int16_t raw_y = (data[2] << 8) | data[3];
    int16_t raw_z = (data[4] << 8) | data[5];

    *ax = raw_x / MPU6050_ACCEL_SCALE;
    *ay = raw_y / MPU6050_ACCEL_SCALE;
    *az = raw_z / MPU6050_ACCEL_SCALE;
}
