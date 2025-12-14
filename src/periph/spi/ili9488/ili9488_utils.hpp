#ifndef ILI9488_UTILS_HPP
#define ILI9488_UTILS_HPP

#include "driver/lgfx_setup.hpp"
#include "ili9488_constants.hpp"

extern LGFX_ILI9488 tft;

unsigned long millis();
int rnd(int min, int max);

#endif // ILI9488_UTILS_HPP
