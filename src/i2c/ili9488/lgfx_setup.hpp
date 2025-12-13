#pragma once
#include <LovyanGFX.hpp>
#include "driver/gpio.h"

class LGFX_ILI9488 : public lgfx::LGFX_Device {
    lgfx::Bus_SPI _bus;
    lgfx::Panel_ILI9488 _panel;
public:

    LGFX_ILI9488(void) {
        // SPI bus configuration
        auto bus_cfg = _bus.config();
        bus_cfg.spi_host    = SPI2_HOST;   // VSPI or HSPI
        bus_cfg.spi_mode    = 0;
        bus_cfg.freq_write  = 40000000;    // 40 MHz
        bus_cfg.freq_read   = 16000000;    // 16 MHz read
        bus_cfg.pin_sclk    = 18;
        bus_cfg.pin_mosi    = 23;
        bus_cfg.pin_miso    = 19;
        bus_cfg.pin_dc      = 2;
        _bus.config(bus_cfg);
        _panel.setBus(&_bus);

        // Panel configuration
        auto panel_cfg = _panel.config();
        panel_cfg.pin_cs    = 15;
        panel_cfg.pin_rst   = 4;
        panel_cfg.pin_busy  = -1;
        panel_cfg.panel_width  = 320;
        panel_cfg.panel_height = 480;
        panel_cfg.memory_width  = 320;
        panel_cfg.memory_height = 480;
        panel_cfg.offset_x = 0;
        panel_cfg.offset_y = 0;
        panel_cfg.readable = true; // set to false if MISO not connected
        _panel.config(panel_cfg);

        setPanel(&_panel);

        // Backlight setup using ESP-IDF GPIO
        gpio_config_t io_conf = {};
        io_conf.pin_bit_mask = (1ULL << 32);
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        io_conf.intr_type = GPIO_INTR_DISABLE;
        gpio_config(&io_conf);

        gpio_set_level((gpio_num_t)32, 1); // turn backlight ON
    }
};
