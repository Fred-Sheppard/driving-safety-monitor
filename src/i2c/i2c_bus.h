#pragma once
#include "esp_err.h"
#include <stdint.h>

#define I2C_MASTER_NUM      I2C_NUM_0
#define I2C_MASTER_SDA      26
#define I2C_MASTER_SCL      25
#define I2C_MASTER_FREQ_HZ  400000
#define I2C_TIMEOUT_MS      1000

esp_err_t i2c_bus_init(void);

esp_err_t i2c_bus_write_byte(uint8_t dev_addr, uint8_t reg, uint8_t data);
esp_err_t i2c_bus_read_bytes(uint8_t dev_addr, uint8_t reg, uint8_t *data, size_t len);
