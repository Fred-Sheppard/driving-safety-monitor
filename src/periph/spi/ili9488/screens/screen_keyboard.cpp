#include "screens.hpp"
#include "../ili9488_utils.hpp"
#include "display/display_manager_utils.hpp"
#include "display/display_manager.hpp"
#include "wifi/wifi_manager.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "screen_keyboard";

#define KEY_W 40
#define KEY_H 38
#define KEY_GAP 2
#define KEY_STEP (KEY_W + KEY_GAP)
#define KB_START_Y 88
#define INPUT_Y 48
#define INPUT_H 32
#define DEBOUNCE_MS 180

static char s_input[65] = {0};
static int s_input_len = 0;
static bool s_shift = false;
static unsigned long s_lastTouch = 0;

static const char *ROWS[] = {"1234567890", "qwertyuiop", "asdfghjkl", "zxcvbnm"};
static const char *ROWS_SHIFT[] = {"!@#$%^&*()", "QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM"};
static const int ROW_LENS[] = {10, 10, 9, 7};

const char *getKeyboardInput() { return s_input; }

static bool debounceOk()
{
  unsigned long now = millis();
  if (now - s_lastTouch < DEBOUNCE_MS)
    return false;
  s_lastTouch = now;
  return true;
}

static int getRowX(int row)
{
  int rowW = ROW_LENS[row] * KEY_STEP - KEY_GAP;
  return (SCREEN_WIDTH - rowW) / 2;
}

static int getRowY(int row)
{
  return KB_START_Y + row * (KEY_H + KEY_GAP);
}

static void drawKey(int x, int y, char c, int w, uint16_t color)
{
  tft.fillRoundRect(x, y, w, KEY_H, 4, color);
  tft.setTextColor(TFT_WHITE, color);
  tft.setTextSize(2);
  tft.setCursor(x + (w - 12) / 2, y + (KEY_H - 16) / 2);
  tft.print(c);
}

static void drawTextKey(int x, int y, const char *txt, int w, uint16_t color)
{
  tft.fillRoundRect(x, y, w, KEY_H, 4, color);
  tft.setTextColor(TFT_WHITE, color);
  tft.setTextSize(1);
  int tw = strlen(txt) * 6;
  tft.setCursor(x + (w - tw) / 2, y + (KEY_H - 8) / 2);
  tft.print(txt);
}

static void drawInputField()
{
  tft.fillRect(10, INPUT_Y, SCREEN_WIDTH - 20, INPUT_H, TFT_BLACK);
  tft.drawRect(10, INPUT_Y, SCREEN_WIDTH - 20, INPUT_H, TFT_WHITE);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(15, INPUT_Y + 8);

  // Show asterisks except last char
  for (int i = 0; i < s_input_len - 1 && i < 26; i++)
    tft.print('*');
  if (s_input_len > 0 && s_input_len <= 26)
    tft.print(s_input[s_input_len - 1]);
  tft.print('_');
}

static void drawKeyboard()
{
  const char **rows = s_shift ? ROWS_SHIFT : ROWS;

  for (int r = 0; r < 3; r++)
  {
    int x = getRowX(r);
    int y = getRowY(r);
    for (int i = 0; i < ROW_LENS[r]; i++)
    {
      drawKey(x, y, rows[r][i], KEY_W, TFT_DARKGREY);
      x += KEY_STEP;
    }
  }

  int delX = getRowX(2) + 9 * KEY_STEP;
  drawTextKey(delX, getRowY(2), "DEL", 42, TFT_RED);

  int y3 = getRowY(3);
  drawTextKey(8, y3, "SHF", 42, s_shift ? TFT_BLUE : TFT_DARKGREY);

  int zx = 54;
  for (int i = 0; i < 7; i++)
  {
    drawKey(zx, y3, rows[3][i], KEY_W, TFT_DARKGREY);
    zx += KEY_STEP;
  }

  drawTextKey(zx + 4, y3, "SPC", 56, TFT_DARKGREY);
  drawTextKey(zx + 64, y3, "OK", 48, TFT_GREEN);
}

void drawKeyboardScreen()
{
  ESP_LOGI(TAG, "Drawing keyboard for: %s", getSelectedSsid());

  s_input[0] = '\0';
  s_input_len = 0;
  s_shift = false;

  tft.fillScreen(COLOR_DARK_BG);

  tft.fillRect(0, 0, SCREEN_WIDTH, 45, TFT_BLACK);
  tft.fillRect(8, 8, 32, 30, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.setTextSize(2);
  tft.setCursor(15, 14);
  tft.print("<");

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(50, 12);
  tft.print("WiFi: ");
  tft.print(getSelectedSsid());

  drawInputField();
  drawKeyboard();
}

static void typeChar(char c)
{
  if (s_input_len < 64)
  {
    s_input[s_input_len++] = c;
    s_input[s_input_len] = '\0';
    drawInputField();
  }
}

bool handleKeyboardScreenTouch()
{
  // Back button (no debounce for navigation)
  if (checkButtonTouch(8, 8, 32, 30))
  {
    ESP_LOGI(TAG, "Back");
    triggerWifiScanScreen();
    return true;
  }

  // Debounce all key presses
  if (!debounceOk())
    return false;

  const char **rows = s_shift ? ROWS_SHIFT : ROWS;

  for (int r = 0; r < 3; r++)
  {
    int x = getRowX(r);
    int y = getRowY(r);
    for (int i = 0; i < ROW_LENS[r]; i++)
    {
      if (checkButtonTouch(x, y, KEY_W, KEY_H))
      {
        typeChar(rows[r][i]);
        return false;
      }
      x += KEY_STEP;
    }
  }

  int delX = getRowX(2) + 9 * KEY_STEP;
  if (checkButtonTouch(delX, getRowY(2), 42, KEY_H))
  {
    if (s_input_len > 0)
    {
      s_input[--s_input_len] = '\0';
      drawInputField();
    }
    return false;
  }

  int y3 = getRowY(3);

  if (checkButtonTouch(8, y3, 42, KEY_H))
  {
    s_shift = !s_shift;
    drawKeyboard();
    return false;
  }

  int zx = 54;
  for (int i = 0; i < 7; i++)
  {
    if (checkButtonTouch(zx, y3, KEY_W, KEY_H))
    {
      typeChar(rows[3][i]);
      return false;
    }
    zx += KEY_STEP;
  }

  if (checkButtonTouch(zx + 4, y3, 56, KEY_H))
  {
    typeChar(' ');
    return false;
  }

  if (checkButtonTouch(zx + 64, y3, 48, KEY_H))
  {
    ESP_LOGI(TAG, "Connecting (pwd len=%d)", s_input_len);
    wifi_manager_connect(getSelectedSsid(), s_input);
    returnToMainScreen();
    return true;
  }

  return false;
}
