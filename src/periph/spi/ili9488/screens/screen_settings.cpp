#include "screens.hpp"
#include "../ili9488_utils.hpp"
#include "display/display_manager_utils.hpp"
#include "display/display_manager.hpp"
#include "config.h"
#include "esp_log.h"

static const char *TAG = "screen_settings";

// Layout constants
#define MENU_START_Y 70
#define MENU_ITEM_HEIGHT 60
#define MENU_ITEM_MARGIN 10
#define BACK_BTN_SIZE 40

// Menu item positions
#define DEVICE_NAME_Y MENU_START_Y
#define WIFI_BTN_Y (MENU_START_Y + MENU_ITEM_HEIGHT + MENU_ITEM_MARGIN)
#define TOUCH_TEST_Y (WIFI_BTN_Y + MENU_ITEM_HEIGHT + MENU_ITEM_MARGIN)

static void drawBackButton()
{
  // Back arrow in top left
  tft.fillRect(10, 10, BACK_BTN_SIZE, BACK_BTN_SIZE, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.setTextSize(3);
  tft.setCursor(18, 18);
  tft.print("<");
}

static void drawMenuItem(int y, const char *label, const char *value, bool isButton)
{
  int itemW = SCREEN_WIDTH - 40;
  uint16_t bgColor = isButton ? TFT_DARKGREY : COLOR_GREY_BG;

  tft.fillRoundRect(20, y, itemW, MENU_ITEM_HEIGHT, 8, bgColor);
  tft.setTextColor(TFT_WHITE, bgColor);
  tft.setTextSize(2);
  tft.setCursor(35, y + 10);
  tft.print(label);

  if (value)
  {
    tft.setTextColor(TFT_LIGHTGREY, bgColor);
    tft.setTextSize(2);
    tft.setCursor(35, y + 35);
    tft.print(value);
  }

  // Draw arrow for buttons
  if (isButton)
  {
    tft.setTextColor(TFT_WHITE, bgColor);
    tft.setTextSize(2);
    tft.setCursor(SCREEN_WIDTH - 60, y + 20);
    tft.print(">");
  }
}

void drawSettingsScreen()
{
  ESP_LOGI(TAG, "Drawing settings screen");

  tft.fillScreen(COLOR_DARK_BG);

  // Header
  tft.fillRect(0, 0, SCREEN_WIDTH, 55, TFT_BLACK);
  drawBackButton();
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(3);
  tft.setCursor(70, 15);
  tft.print("Settings");

  // Device name (display only)
  drawMenuItem(DEVICE_NAME_Y, "Device Name", DEVICE_NAME, false);

  // WiFi selection (button)
  drawMenuItem(WIFI_BTN_Y, "WiFi Network", "Tap to scan", true);

  // Touch test (button)
  drawMenuItem(TOUCH_TEST_Y, "Touch Test", "Calibration test", true);
}

bool handleSettingsScreenTouch()
{
  // Back button
  if (checkButtonTouch(10, 10, BACK_BTN_SIZE, BACK_BTN_SIZE))
  {
    ESP_LOGI(TAG, "Back button pressed");
    returnToMainScreen();
    return true;
  }

  // WiFi button
  if (checkButtonTouch(20, WIFI_BTN_Y, SCREEN_WIDTH - 40, MENU_ITEM_HEIGHT))
  {
    ESP_LOGI(TAG, "WiFi button pressed");
    triggerWifiScanScreen();
    return true;
  }

  // Touch test button
  if (checkButtonTouch(20, TOUCH_TEST_Y, SCREEN_WIDTH - 40, MENU_ITEM_HEIGHT))
  {
    ESP_LOGI(TAG, "Touch test button pressed");
    triggerTouchTestScreen();
    return true;
  }

  return false;
}
