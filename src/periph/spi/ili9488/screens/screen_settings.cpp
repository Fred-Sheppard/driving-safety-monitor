#include "screens.hpp"
#include "../ili9488_utils.hpp"
#include "display/display_manager_utils.hpp"
#include "display/display_manager.hpp"
#include "wifi/wifi_manager.h"
#include "config.h"
#include "esp_log.h"
#include "esp_mac.h"

static const char *TAG = "screen_settings";

// Layout constants
#define MENU_START_Y 70
#define MENU_ITEM_HEIGHT 60
#define MENU_ITEM_MARGIN 10
#define BACK_BTN_SIZE 40

// Menu item positions
#define DEVICE_NAME_Y MENU_START_Y
#define WIFI_BTN_Y (MENU_START_Y + MENU_ITEM_HEIGHT + MENU_ITEM_MARGIN)
#define DISCONNECT_BTN_Y (WIFI_BTN_Y + MENU_ITEM_HEIGHT + MENU_ITEM_MARGIN)
#define TOUCH_TEST_Y_CONNECTED (DISCONNECT_BTN_Y + MENU_ITEM_HEIGHT + MENU_ITEM_MARGIN)
#define TOUCH_TEST_Y_DISCONNECTED (WIFI_BTN_Y + MENU_ITEM_HEIGHT + MENU_ITEM_MARGIN)

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

  // Device name with MAC address
  uint8_t mac[6];
  char macStr[18];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  drawMenuItem(DEVICE_NAME_Y, "Device Name", macStr, false);

  // WiFi section - check connection status
  char ssid[WIFI_SSID_MAX_LEN];
  bool connected = wifi_manager_get_ssid(ssid);

  if (connected)
  {
    // Show "Connected to: SSID" as display item
    char connectedText[50];
    snprintf(connectedText, sizeof(connectedText), "Connected to: %s", ssid);
    drawMenuItem(WIFI_BTN_Y, "WiFi Network", connectedText, false);

    // Disconnect button
    drawMenuItem(DISCONNECT_BTN_Y, "Disconnect", "Tap to disconnect", true);

    // Touch test (button) - shifted down
    drawMenuItem(TOUCH_TEST_Y_CONNECTED, "Touch Test", "Calibration test", true);
  }
  else
  {
    // WiFi selection (button)
    drawMenuItem(WIFI_BTN_Y, "WiFi Network", "Tap to scan", true);

    // Touch test (button)
    drawMenuItem(TOUCH_TEST_Y_DISCONNECTED, "Touch Test", "Calibration test", true);
  }
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

  bool connected = wifi_manager_is_connected();

  if (connected)
  {
    // Disconnect button
    if (checkButtonTouch(20, DISCONNECT_BTN_Y, SCREEN_WIDTH - 40, MENU_ITEM_HEIGHT))
    {
      ESP_LOGI(TAG, "Disconnect button pressed");
      wifi_manager_disconnect();
      drawSettingsScreen(); // Redraw to show updated state
      return true;
    }

    // Touch test button (shifted down when connected)
    if (checkButtonTouch(20, TOUCH_TEST_Y_CONNECTED, SCREEN_WIDTH - 40, MENU_ITEM_HEIGHT))
    {
      ESP_LOGI(TAG, "Touch test button pressed");
      triggerTouchTestScreen();
      return true;
    }
  }
  else
  {
    // WiFi button (only when not connected)
    if (checkButtonTouch(20, WIFI_BTN_Y, SCREEN_WIDTH - 40, MENU_ITEM_HEIGHT))
    {
      ESP_LOGI(TAG, "WiFi button pressed");
      triggerWifiScanScreen();
      return true;
    }

    // Touch test button
    if (checkButtonTouch(20, TOUCH_TEST_Y_DISCONNECTED, SCREEN_WIDTH - 40, MENU_ITEM_HEIGHT))
    {
      ESP_LOGI(TAG, "Touch test button pressed");
      triggerTouchTestScreen();
      return true;
    }
  }

  return false;
}
