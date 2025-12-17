#include "screens.hpp"
#include "../ili9488_utils.hpp"
#include "display/display_manager.hpp"
#include "esp_log.h"
#include <stdio.h>

static const char *TAG = "touch_test";

void drawTouchTestScreen()
{
  ESP_LOGI(TAG, "Touch test screen - tap anywhere");
  tft.fillScreen(TFT_WHITE);

  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.print("TOUCH TEST - Tap anywhere");
  tft.setCursor(10, 35);
  tft.print("Back button: top-left corner");

  // Draw corner markers
  tft.fillRect(0, 0, 20, 20, TFT_RED);        // Top-left (back)
  tft.fillRect(460, 0, 20, 20, TFT_GREEN);    // Top-right
  tft.fillRect(0, 300, 20, 20, TFT_BLUE);     // Bottom-left
  tft.fillRect(460, 300, 20, 20, TFT_YELLOW); // Bottom-right

  tft.setTextSize(1);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.setCursor(25, 5);
  tft.print("TL(0,0)");
  tft.setCursor(400, 5);
  tft.print("TR(480,0)");
  tft.setCursor(25, 305);
  tft.print("BL(0,320)");
  tft.setCursor(400, 305);
  tft.print("BR(480,320)");
}

bool handleTouchTestScreenTouch()
{
  int32_t x, y;
  if (tft.getTouch(&x, &y))
  {
    // Draw dot at RAW touch position (no flip)
    tft.fillCircle(x, y, 8, TFT_MAGENTA);

    // Show coordinates
    char buf[40];
    snprintf(buf, sizeof(buf), "Raw: %ld,%ld", x, y);
    tft.fillRect(150, 280, 180, 30, TFT_WHITE);
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(150, 285);
    tft.print(buf);

    ESP_LOGI(TAG, "Touch at raw x=%ld y=%ld", x, y);

    if (x < 50 && y < 50)
    {
      ESP_LOGI(TAG, "Back pressed");
      returnToMainScreen();
      return true;
    }
  }
  return false;
}
