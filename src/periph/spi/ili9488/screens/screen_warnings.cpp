#include "screens.hpp"
#include "../ili9488_utils.hpp"
#include "esp_log.h"

static const char *TAG = "screen_warnings";

void drawWarningCountdown(int countdown, bool fullRedraw)
{
  ESP_LOGI(TAG, "Drawing warning countdown: %d", countdown);

  if (fullRedraw)
  {
    tft.fillScreen(COLOR_DARK_BG);
    tft.fillRoundRect(DIALOG_BOX_X, DIALOG_BOX_Y, DIALOG_BOX_WIDTH, DIALOG_BOX_HEIGHT, 10, TFT_RED);
    tft.fillRoundRect(DIALOG_BOX_X + 5, DIALOG_BOX_Y + 5, DIALOG_BOX_WIDTH - 10, DIALOG_BOX_HEIGHT - 10, 8, TFT_WHITE);

    int iconX = SCREEN_WIDTH / 2;
    int iconY = DIALOG_BOX_Y + 50;
    tft.fillTriangle(iconX, iconY - 25, iconX - 30, iconY + 25, iconX + 30, iconY + 25, TFT_RED);
    tft.fillRect(iconX - 4, iconY - 15, 8, 25, TFT_WHITE);
    tft.fillCircle(iconX, iconY + 18, 5, TFT_WHITE);

    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    tft.setTextSize(3);
    tft.setCursor(DIALOG_BOX_X + 40, DIALOG_BOX_Y + 100);
    tft.println("COLLISION WARNING!");

    int btnX = (SCREEN_WIDTH - BUTTON_WIDTH) / 2;
    int btnY = DIALOG_BOX_Y + DIALOG_BOX_HEIGHT - 70;
    tft.fillRoundRect(btnX, btnY, BUTTON_WIDTH, BUTTON_HEIGHT, BUTTON_RADIUS, TFT_GREEN);
    tft.setTextSize(3);
    tft.setTextColor(TFT_WHITE, TFT_GREEN);
    tft.setCursor(btnX + 15, btnY + 20);
    tft.println("PRESS TO CANCEL");
  }

  tft.fillRect(200, DIALOG_BOX_Y + 130, 100, 50, TFT_WHITE);
  tft.setTextSize(6);
  tft.setCursor(220, DIALOG_BOX_Y + 130);
  tft.setTextColor(TFT_RED, TFT_WHITE);
  tft.println(countdown);
}

void drawCrashScreen()
{
  ESP_LOGI(TAG, "Drawing crash screen");
  tft.fillScreen(COLOR_DARK_RED_BG);

  int boxX = (SCREEN_WIDTH - DIALOG_BOX_WIDTH) / 2;
  int boxY = (SCREEN_HEIGHT - CRASH_BOX_HEIGHT) / 2;

  tft.fillRoundRect(boxX, boxY, DIALOG_BOX_WIDTH, CRASH_BOX_HEIGHT, 12, TFT_BLACK);
  tft.fillRoundRect(boxX + 5, boxY + 5, DIALOG_BOX_WIDTH - 10, CRASH_BOX_HEIGHT - 10, 10, TFT_WHITE);

  int iconX = SCREEN_WIDTH / 2;
  int iconY = boxY + 60;
  tft.fillCircle(iconX, iconY, 40, TFT_RED);
  tft.drawLine(iconX - 25, iconY - 25, iconX + 25, iconY + 25, TFT_WHITE);
  tft.drawLine(iconX + 25, iconY - 25, iconX - 25, iconY + 25, TFT_WHITE);
  tft.drawLine(iconX - 24, iconY - 25, iconX + 24, iconY + 25, TFT_WHITE);
  tft.drawLine(iconX + 24, iconY - 25, iconX - 24, iconY + 25, TFT_WHITE);

  tft.setTextColor(TFT_RED, TFT_WHITE);
  tft.setTextSize(4);
  tft.setCursor(boxX + 120, boxY + 130);
  tft.println("CRASH!");

  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(boxX + 70, boxY + 180);
  tft.println("Emergency services");
  tft.setCursor(boxX + 130, boxY + 210);
  tft.println("contacted");

  tft.fillCircle(boxX + 30, boxY + 250, 8, TFT_RED);
  tft.setTextSize(1);
  tft.setCursor(boxX + 50, boxY + 245);
  tft.println("Recording incident data...");
}

void drawCautionScreen()
{
  ESP_LOGI(TAG, "Drawing caution screen");
  tft.fillScreen(COLOR_GREY_BG);

  tft.fillRect(0, 0, SCREEN_WIDTH, COLOR_BANNER_HEIGHT, TFT_ORANGE);

  int iconX = 80;
  int iconY = 70;
  tft.fillTriangle(iconX, iconY - 30, iconX - 35, iconY + 30, iconX + 35, iconY + 30, TFT_YELLOW);
  tft.fillRect(iconX - 5, iconY - 20, 10, 30, TFT_BLACK);
  tft.fillCircle(iconX, iconY + 20, 5, TFT_BLACK);

  tft.setTextColor(TFT_BLACK, TFT_ORANGE);
  tft.setTextSize(3);
  tft.setCursor(160, 30);
  tft.println("CAUTION");

  tft.setTextSize(2);
  tft.setCursor(160, 70);
  tft.println("Vehicle detected in");
  tft.setCursor(160, 100);
  tft.println("blind spot");

  int btnWidth = 250;
  int btnX = (SCREEN_WIDTH - btnWidth) / 2;
  int btnY = 220;

  tft.fillRoundRect(btnX, btnY, btnWidth, BUTTON_HEIGHT, BUTTON_RADIUS, TFT_WHITE);
  tft.drawRoundRect(btnX, btnY, btnWidth, BUTTON_HEIGHT, BUTTON_RADIUS, TFT_ORANGE);
  tft.drawRoundRect(btnX + 1, btnY + 1, btnWidth - 2, BUTTON_HEIGHT - 2, BUTTON_RADIUS, TFT_ORANGE);
  tft.setTextSize(3);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.setCursor(btnX + 50, btnY + 20);
  tft.println("DISMISS");
}
