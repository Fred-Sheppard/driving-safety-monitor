#include "screens.hpp"
#include "../ili9488_utils.hpp"
#include "display/display_manager_utils.hpp"
#include "display/display_manager.hpp"
#include "wifi/wifi_manager.h"
#include "esp_log.h"

static const char *TAG = "screen_wifi";

#define HEADER_H 55
#define BACK_BTN_SIZE 40
#define LIST_START_Y (HEADER_H + 5)
#define LIST_ITEM_H 45
#define MAX_VISIBLE_ITEMS 5

static wifi_scan_result_t s_results[WIFI_MAX_SCAN_RESULTS];
static uint16_t s_result_count = 0;
static int s_scroll_offset = 0;
static bool s_scanning = false;
static char s_selected_ssid[WIFI_SSID_MAX_LEN] = {0};

void setSelectedSsid(const char *ssid)
{
  strncpy(s_selected_ssid, ssid, WIFI_SSID_MAX_LEN - 1);
  s_selected_ssid[WIFI_SSID_MAX_LEN - 1] = '\0';
}

const char *getSelectedSsid()
{
  return s_selected_ssid;
}

static void drawSignalBars(int x, int y, int rssi)
{
  int bars = (rssi > -50) ? 4 : (rssi > -60) ? 3
                             : (rssi > -70)   ? 2
                                              : 1;
  for (int i = 0; i < 4; i++)
  {
    int h = 5 + i * 4;
    uint16_t col = (i < bars) ? TFT_GREEN : TFT_DARKGREY;
    tft.fillRect(x + i * 6, y + (20 - h), 4, h, col);
  }
}

static void drawNetworkList()
{
  int y = LIST_START_Y;
  int itemW = SCREEN_WIDTH - 20;

  for (int i = 0; i < MAX_VISIBLE_ITEMS && (i + s_scroll_offset) < s_result_count; i++)
  {
    int idx = i + s_scroll_offset;
    tft.fillRect(10, y, itemW, LIST_ITEM_H - 2, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
    tft.setTextSize(2);
    tft.setCursor(20, y + 12);

    char ssid_disp[20];
    strncpy(ssid_disp, s_results[idx].ssid, 19);
    ssid_disp[19] = '\0';
    tft.print(ssid_disp);

    drawSignalBars(SCREEN_WIDTH - 50, y + 10, s_results[idx].rssi);

    if (s_results[idx].authmode != 0)
    {
      tft.setTextSize(1);
      tft.setCursor(SCREEN_WIDTH - 80, y + 16);
      tft.print("*");
    }

    y += LIST_ITEM_H;
  }

  if (s_scroll_offset > 0)
  {
    tft.fillTriangle(240, LIST_START_Y - 10, 230, LIST_START_Y - 2,
                     250, LIST_START_Y - 2, TFT_WHITE);
  }
  if (s_scroll_offset + MAX_VISIBLE_ITEMS < s_result_count)
  {
    int by = LIST_START_Y + MAX_VISIBLE_ITEMS * LIST_ITEM_H + 5;
    tft.fillTriangle(240, by + 10, 230, by, 250, by, TFT_WHITE);
  }
}

void drawWifiScanScreen()
{
  ESP_LOGI(TAG, "Drawing WiFi scan screen");

  tft.fillScreen(COLOR_DARK_BG);

  tft.fillRect(0, 0, SCREEN_WIDTH, HEADER_H, TFT_BLACK);
  tft.fillRect(10, 10, BACK_BTN_SIZE, BACK_BTN_SIZE, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.setTextSize(3);
  tft.setCursor(18, 18);
  tft.print("<");

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(70, 18);
  tft.print("WiFi Networks");

  s_scanning = true;
  s_result_count = 0;
  s_scroll_offset = 0;
  wifi_manager_start_scan();

  tft.setTextColor(TFT_LIGHTGREY, COLOR_DARK_BG);
  tft.setTextSize(2);
  tft.setCursor(150, 150);
  tft.print("Scanning...");
}

bool handleWifiScanScreenTouch()
{
  if (s_scanning && wifi_manager_scan_done())
  {
    s_scanning = false;
    s_result_count = wifi_manager_get_scan_results(s_results, WIFI_MAX_SCAN_RESULTS);
    ESP_LOGI(TAG, "Scan found %d networks", s_result_count);

    tft.fillRect(0, LIST_START_Y - 15, SCREEN_WIDTH, SCREEN_HEIGHT - LIST_START_Y + 15, COLOR_DARK_BG);
    if (s_result_count > 0)
      drawNetworkList();
    else
    {
      tft.setTextColor(TFT_LIGHTGREY, COLOR_DARK_BG);
      tft.setTextSize(2);
      tft.setCursor(140, 150);
      tft.print("No networks found");
    }
  }

  if (checkButtonTouch(10, 10, BACK_BTN_SIZE, BACK_BTN_SIZE))
  {
    ESP_LOGI(TAG, "Back pressed");
    triggerSettingsScreen();
    return true;
  }

  if (s_scroll_offset > 0 && checkButtonTouch(200, LIST_START_Y - 20, 80, 20))
  {
    s_scroll_offset--;
    tft.fillRect(0, LIST_START_Y - 15, SCREEN_WIDTH, SCREEN_HEIGHT - LIST_START_Y + 15, COLOR_DARK_BG);
    drawNetworkList();
  }

  if (s_scroll_offset + MAX_VISIBLE_ITEMS < s_result_count)
  {
    int by = LIST_START_Y + MAX_VISIBLE_ITEMS * LIST_ITEM_H;
    if (checkButtonTouch(200, by, 80, 20))
    {
      s_scroll_offset++;
      tft.fillRect(0, LIST_START_Y - 15, SCREEN_HEIGHT - LIST_START_Y + 15, SCREEN_HEIGHT - LIST_START_Y + 15, COLOR_DARK_BG);
      drawNetworkList();
    }
  }

  if (!s_scanning && s_result_count > 0)
  {
    for (int i = 0; i < MAX_VISIBLE_ITEMS && (i + s_scroll_offset) < s_result_count; i++)
    {
      int y = LIST_START_Y + i * LIST_ITEM_H;
      if (checkButtonTouch(10, y, SCREEN_WIDTH - 20, LIST_ITEM_H - 2))
      {
        int idx = i + s_scroll_offset;
        ESP_LOGI(TAG, "Selected: %s", s_results[idx].ssid);
        setSelectedSsid(s_results[idx].ssid);
        triggerKeyboardScreen(s_results[idx].ssid);
        return true;
      }
    }
  }

  return false;
}
