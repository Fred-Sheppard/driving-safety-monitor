#include "screens.hpp"
#include "map_elements.hpp"
#include "../ili9488_utils.hpp"
#include "display/display_manager_utils.hpp"
#include "display/display_manager.hpp"
#include "esp_log.h"
#include <math.h>

static const char *TAG = "screen_main";

// Settings button position (gear icon in top right)
#define SETTINGS_BTN_X 420
#define SETTINGS_BTN_Y 5
#define SETTINGS_BTN_SIZE 40

static void drawGrass()
{
  tft.fillScreen(COLOR_GRASS_GREEN);
  for (int i = 0; i < 40; i++)
    tft.fillCircle(rnd(0, SCREEN_WIDTH), rnd(0, SCREEN_HEIGHT), rnd(5, 15), COLOR_DARK_GREEN);
}

static void drawSettingsIcon(int cx, int cy, int r)
{
  tft.fillCircle(cx, cy, r - 4, TFT_WHITE);
  tft.fillCircle(cx, cy, r - 8, TFT_BLACK);
  for (int i = 0; i < 8; i++)
  {
    float angle = i * 3.14159f / 4.0f;
    int x1 = cx + (r - 2) * cos(angle);
    int y1 = cy + (r - 2) * sin(angle);
    tft.fillCircle(x1, y1, 4, TFT_WHITE);
  }
}

static void drawHeader()
{
  tft.fillRect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 20);
  tft.println("GPS VIEW");

  drawSettingsIcon(SETTINGS_BTN_X + SETTINGS_BTN_SIZE / 2,
                   SETTINGS_BTN_Y + SETTINGS_BTN_SIZE / 2, 18);
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

bool handleMainScreenTouch()
{
  if (checkButtonTouch(SETTINGS_BTN_X, SETTINGS_BTN_Y,
                       SETTINGS_BTN_SIZE, SETTINGS_BTN_SIZE))
  {
    ESP_LOGI(TAG, "Settings button pressed");
    triggerSettingsScreen();
    return true;
  }
  return false;
}
