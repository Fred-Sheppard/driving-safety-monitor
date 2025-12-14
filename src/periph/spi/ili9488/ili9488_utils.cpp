#include "ili9488_utils.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <random>

LGFX_ILI9488 tft;

unsigned long millis()
{
  return (unsigned long)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

int rnd(int min, int max)
{
  static std::mt19937 gen(std::random_device{}());
  std::uniform_int_distribution<> dist(min, max - 1);
  return dist(gen);
}
