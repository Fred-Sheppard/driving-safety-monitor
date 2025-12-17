#include "i2c_bus.h"
#include "driver/i2c.h"
#include "esp_log.h"

static const char *TAG = "i2c_bus";

esp_err_t i2c_bus_init(void) {
    esp_err_t ret;

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

    ret = i2c_driver_install(I2C_MASTER_NUM, I2C_MODE_MASTER, 0, 0, 0);
    if (ret == ESP_ERR_INVALID_STATE) {
        ESP_LOGI(TAG, "I2C already initialized");
        return ESP_OK;
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "I2C bus initialised");
    return ESP_OK;
}

esp_err_t i2c_bus_write_byte(uint8_t dev_addr, uint8_t reg, uint8_t data) {
    uint8_t buf[2] = {reg, data};
    return i2c_master_write_to_device(
        I2C_MASTER_NUM, dev_addr, buf, sizeof(buf),
        pdMS_TO_TICKS(I2C_TIMEOUT_MS)
    );
}

esp_err_t i2c_bus_read_bytes(uint8_t dev_addr, uint8_t reg, uint8_t *data, size_t len) {
    return i2c_master_write_read_device(
        I2C_MASTER_NUM, dev_addr, &reg, 1, data, len,
        pdMS_TO_TICKS(I2C_TIMEOUT_MS)
    );
}
