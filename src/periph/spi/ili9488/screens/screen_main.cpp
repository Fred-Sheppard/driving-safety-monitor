#include "screens.hpp"
#include "map_elements.hpp"
#include "../ili9488_utils.hpp"
#include "esp_log.h"

static const char *TAG = "screen_main";

static void drawGrass()
{
  tft.fillScreen(COLOR_GRASS_GREEN);
  for (int i = 0; i < 40; i++)
    tft.fillCircle(rnd(0, SCREEN_WIDTH), rnd(0, SCREEN_HEIGHT), rnd(5, 15), COLOR_DARK_GREEN);
}

static void drawHeader()
{
  tft.fillRect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 20);
  tft.println("GPS VIEW");

  tft.drawCircle(430, 25, 12, TFT_WHITE);
  tft.fillTriangle(430, 15, 426, 23, 434, 23, TFT_WHITE);
  tft.setTextSize(1);
  tft.setCursor(426, 10);
  tft.println("N");
}

static void drawFooter()
{
  tft.fillRect(0, SCREEN_HEIGHT - FOOTER_HEIGHT, SCREEN_WIDTH, FOOTER_HEIGHT, TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, SCREEN_HEIGHT - 40);
  tft.println("Lat: 51.5074");
  tft.setCursor(10, SCREEN_HEIGHT - 20);
  tft.println("Lon: -0.1278");
}

void drawMainScreen()
{
  ESP_LOGI(TAG, "Drawing main screen");
  drawGrass();
  drawRoad();
  drawTrees();
  drawCar();
  drawHeader();
  drawFooter();
}
