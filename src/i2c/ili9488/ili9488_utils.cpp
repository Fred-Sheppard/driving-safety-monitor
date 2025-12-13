#include "ili9488_utils.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_random.h"
#include <random>

LGFX_ILI9488 tft;

uint16_t GRASS_GREEN;
uint16_t DARK_GREEN;
uint16_t ROAD_GREY;
uint16_t ROAD_DARK;
uint16_t TREE_GREEN;
uint16_t BROWN;
uint16_t CAR_BLUE;

void initColors() {
  GRASS_GREEN = tft.color565(76, 175, 80);
  DARK_GREEN = tft.color565(46, 125, 50);
  ROAD_GREY = tft.color565(117, 117, 117);
  ROAD_DARK = tft.color565(66, 66, 66);
  TREE_GREEN = tft.color565(27, 94, 32);
  BROWN = tft.color565(109, 76, 65);
  CAR_BLUE = tft.color565(25, 118, 210);
}

unsigned long millis() {
  return (unsigned long)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

void delay(unsigned long ms) {
  vTaskDelay(pdMS_TO_TICKS(ms));
}

int rnd(int min, int max) { 
  static std::mt19937 gen(std::random_device{}());
  std::uniform_int_distribution<> dist(min, max - 1);
  return dist(gen);
}

void drawTree(int x, int y, int size, uint16_t trunk, uint16_t foliage1, uint16_t foliage2) {
  int trunkWidth = size / 5;
  tft.fillRect(x - trunkWidth / 2, y, trunkWidth, size / 2, trunk);
  
  int radius = size / 2;
  tft.fillCircle(x, y, radius, foliage1);
  tft.fillCircle(x - radius / 2, y - radius / 3, radius / 2 + 2, foliage2);
  tft.fillCircle(x + radius / 2, y - radius / 3, radius / 2 + 2, foliage2);
}

void drawRoadSegment(uint16_t color, int width) {
  int halfWidth = width / 2;
  
  for (int y = 0; y < 60; y += 2) {
    tft.fillCircle(120, y, halfWidth, color);
  }
  
  for (int y = 60; y < 120; y += 2) {
    float t = (y - 60) / 60.0;
    int x = 120 + (60 * t);
    tft.fillCircle(x, y, halfWidth, color);
  }
  
  for (int y = 120; y < 220; y += 2) {
    float t = (y - 120) / 100.0;
    int x = 180 - (120 * t);
    tft.fillCircle(x, y, halfWidth, color);
  }
  
  for (int y = 220; y < 320; y += 2) {
    float t = (y - 220) / 100.0;
    int x = 60 + (60 * t);
    tft.fillCircle(x, y, halfWidth, color);
  }
}

void drawCenterLine() {
  bool draw = true;
  int dashLength = 8;
  int gapLength = 8;
  int counter = 0;
  
  for (int y = 0; y < 60; y++) {
    if (draw && counter < dashLength) {
      tft.drawPixel(120, y, TFT_YELLOW);
      tft.drawPixel(121, y, TFT_YELLOW);
    }
    counter++;
    if (counter >= dashLength + gapLength) counter = 0;
    if (counter == dashLength) draw = false;
    if (counter == 0) draw = true;
  }
  
  counter = 0;
  draw = true;
  for (int y = 60; y < 120; y++) {
    float t = (y - 60) / 60.0;
    int x = 120 + (60 * t);
    if (draw && counter < dashLength) {
      tft.drawPixel(x, y, TFT_YELLOW);
      tft.drawPixel(x + 1, y, TFT_YELLOW);
    }
    counter++;
    if (counter >= dashLength + gapLength) counter = 0;
    if (counter == dashLength) draw = false;
    if (counter == 0) draw = true;
  }
  
  counter = 0;
  draw = true;
  for (int y = 120; y < 220; y++) {
    float t = (y - 120) / 100.0;
    int x = 180 - (120 * t);
    if (draw && counter < dashLength) {
      tft.drawPixel(x, y, TFT_YELLOW);
      tft.drawPixel(x + 1, y, TFT_YELLOW);
    }
    counter++;
    if (counter >= dashLength + gapLength) counter = 0;
    if (counter == dashLength) draw = false;
    if (counter == 0) draw = true;
  }
  
  counter = 0;
  draw = true;
  for (int y = 220; y < 320; y++) {
    float t = (y - 220) / 100.0;
    int x = 60 + (60 * t);
    if (draw && counter < dashLength) {
      tft.drawPixel(x, y, TFT_YELLOW);
      tft.drawPixel(x + 1, y, TFT_YELLOW);
    }
    counter++;
    if (counter >= dashLength + gapLength) counter = 0;
    if (counter == dashLength) draw = false;
    if (counter == 0) draw = true;
  }
}
