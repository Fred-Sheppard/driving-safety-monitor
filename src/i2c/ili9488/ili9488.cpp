#include "ili9488.hpp"
#include "ili9488_utils.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_random.h"
#include <cstdlib>
#include "esp_log.h"
#include "trace/trace.h"

static const char *TAG = "ili9488";

AppState currentState = STATE_MAIN;
AppState previousState = STATE_MAIN;
int countdownValue = 10;
unsigned long lastCountdownTime = 0;
unsigned long stateStartTime = 0;
SemaphoreHandle_t stateMutex = NULL;

void tft_init()
{
  // Initialize the mutex if it's not already initialized
  if (stateMutex == NULL)
  {
    stateMutex = xSemaphoreCreateMutex();
    if (stateMutex == NULL)
    {
      ESP_LOGE("TFT", "Failed to create state mutex");
      return;
    }
  }

  tft.init();
  tft.setRotation(1);
  initColors();
  drawMainScreen();
}

void drawMainScreen()
{
  ESP_LOGI(TAG, "Drawing main screen");
  tft.fillScreen(GRASS_GREEN);

  for (int i = 0; i < 40; i++)
  {
    int x = rnd(0, 480);
    int y = rnd(0, 320);
    int size = rnd(5, 15);
    tft.fillCircle(x, y, size, DARK_GREEN);
  }

  drawRoadSegment(ROAD_DARK, 35);
  drawRoadSegment(ROAD_GREY, 30);
  drawCenterLine();

  int numTrees = 25;
  for (int i = 0; i < numTrees; i++)
  {
    int x = rnd(30, 481);
    int y = rnd(50, 301);
    int size = rnd(10, 22);

    bool tooCloseToRoad = false;
    if (y < 60 && abs(x - 120) < 40)
      tooCloseToRoad = true;
    if (y >= 60 && y < 120 && abs(x - (120 + (60 * (y - 60) / 60.0))) < 40)
      tooCloseToRoad = true;
    if (y >= 120 && y < 220 && abs(x - (180 - (120 * (y - 120) / 100.0))) < 40)
      tooCloseToRoad = true;
    if (y >= 220 && abs(x - (60 + (60 * (y - 220) / 100.0))) < 40)
      tooCloseToRoad = true;

    if (!tooCloseToRoad)
    {
      drawTree(x, y, size, BROWN, TREE_GREEN, DARK_GREEN);
    }
  }

  int carX = 180;
  int carY = 120;
  tft.fillRect(carX - 8, carY - 10, 16, 20, CAR_BLUE);
  tft.fillRect(carX - 6, carY - 6, 12, 10, tft.color565(21, 101, 192));
  tft.fillRect(carX - 4, carY - 4, 8, 6, tft.color565(144, 202, 249));

  tft.fillRect(0, 0, 480, 50, TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 20);
  tft.println("GPS VIEW");

  tft.drawCircle(430, 25, 12, TFT_WHITE);
  tft.fillTriangle(430, 15, 426, 23, 434, 23, TFT_WHITE);
  tft.setTextSize(1);
  tft.setCursor(426, 10);
  tft.println("N");

  tft.fillRect(0, 270, 480, 50, TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 280);
  tft.println("Lat: 51.5074");
  tft.setCursor(10, 300);
  tft.println("Lon: -0.1278");
}

void drawWarningCountdown(int countdown, bool fullRedraw)
{
  ESP_LOGI(TAG, "Drawing warning countdown");
  int boxWidth = 400;
  int boxHeight = 250;
  int boxX = (480 - boxWidth) / 2;
  int boxY = (320 - boxHeight) / 2;

  if (fullRedraw)
  {
    tft.fillScreen(tft.color565(20, 20, 20));
    tft.fillRoundRect(boxX, boxY, boxWidth, boxHeight, 10, TFT_RED);
    tft.fillRoundRect(boxX + 5, boxY + 5, boxWidth - 10, boxHeight - 10, 8, TFT_WHITE);

    int iconX = 240;
    int iconY = boxY + 50;
    tft.fillTriangle(iconX, iconY - 25, iconX - 30, iconY + 25, iconX + 30, iconY + 25, TFT_RED);
    tft.fillRect(iconX - 4, iconY - 15, 8, 25, TFT_WHITE);
    tft.fillCircle(iconX, iconY + 18, 5, TFT_WHITE);

    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    tft.setTextSize(3);
    tft.setCursor(boxX + 40, boxY + 100);
    tft.println("COLLISION WARNING!");

    int btnWidth = 300;
    int btnHeight = 60;
    int btnX = (480 - btnWidth) / 2;
    int btnY = boxY + boxHeight - 70;

    tft.fillRoundRect(btnX, btnY, btnWidth, btnHeight, 8, TFT_GREEN);
    tft.setTextSize(3);
    tft.setTextColor(TFT_WHITE, TFT_GREEN);
    tft.setCursor(btnX + 15, btnY + 20);
    tft.println("PRESS TO CANCEL");
  }

  tft.fillRect(200, boxY + 130, 100, 50, TFT_WHITE);
  tft.setTextSize(6);
  tft.setCursor(220, boxY + 130);
  tft.setTextColor(TFT_RED, TFT_WHITE);
  tft.println(countdown);
}

void drawCrashOccurred()
{
  ESP_LOGI(TAG, "Drawing crash occurred screen");
  tft.fillScreen(tft.color565(139, 0, 0));

  int boxWidth = 400;
  int boxHeight = 280;
  int boxX = (480 - boxWidth) / 2;
  int boxY = (320 - boxHeight) / 2;

  tft.fillRoundRect(boxX, boxY, boxWidth, boxHeight, 12, TFT_BLACK);
  tft.fillRoundRect(boxX + 5, boxY + 5, boxWidth - 10, boxHeight - 10, 10, TFT_WHITE);

  int iconX = 240;
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

void drawNormalWarning()
{
  ESP_LOGI(TAG, "Drawing normal warning screen");
  tft.fillScreen(tft.color565(40, 40, 40));

  int bannerHeight = 140;
  tft.fillRect(0, 0, 480, bannerHeight, TFT_ORANGE);

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
  int btnHeight = 60;
  int btnX = (480 - btnWidth) / 2;
  int btnY = 220;

  tft.fillRoundRect(btnX, btnY, btnWidth, btnHeight, 8, TFT_WHITE);
  tft.drawRoundRect(btnX, btnY, btnWidth, btnHeight, 8, TFT_ORANGE);
  tft.drawRoundRect(btnX + 1, btnY + 1, btnWidth - 2, btnHeight - 2, 8, TFT_ORANGE);
  tft.setTextSize(3);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.setCursor(btnX + 50, btnY + 20);
  tft.println("DISMISS");
}

void displayTask(void *pvParameters)
{
  while (1)
  {
    TRACE_TASK_RUN(TAG);
    unsigned long currentTime = millis();

    xSemaphoreTake(stateMutex, portMAX_DELAY);
    AppState state = currentState;
    int countdown = countdownValue;
    bool stateChanged = (state != previousState);
    xSemaphoreGive(stateMutex);

    switch (state)
    {
    case STATE_MAIN:
      if (stateChanged)
      {
        drawMainScreen();
        xSemaphoreTake(stateMutex, portMAX_DELAY);
        previousState = STATE_MAIN;
        stateStartTime = currentTime;
        xSemaphoreGive(stateMutex);
      }
      break;

    case STATE_WARNING_COUNTDOWN:
      if (stateChanged)
      {
        drawWarningCountdown(countdown, true);
        xSemaphoreTake(stateMutex, portMAX_DELAY);
        previousState = STATE_WARNING_COUNTDOWN;
        lastCountdownTime = currentTime;
        xSemaphoreGive(stateMutex);
      }
      else if (currentTime - lastCountdownTime >= 1000)
      {
        xSemaphoreTake(stateMutex, portMAX_DELAY);
        countdownValue--;
        lastCountdownTime = currentTime;

        if (countdownValue <= 0)
        {
          currentState = STATE_CRASH;
        }
        else
        {
          drawWarningCountdown(countdownValue, false);
        }
        xSemaphoreGive(stateMutex);
      }
      break;

    case STATE_CRASH:
      if (stateChanged)
      {
        drawCrashOccurred();
        xSemaphoreTake(stateMutex, portMAX_DELAY);
        previousState = STATE_CRASH;
        stateStartTime = currentTime;
        xSemaphoreGive(stateMutex);
      }
      else if (currentTime - stateStartTime > 5000)
      {
        xSemaphoreTake(stateMutex, portMAX_DELAY);
        currentState = STATE_MAIN;
        xSemaphoreGive(stateMutex);
      }
      break;

    case STATE_NORMAL_WARNING:
      if (stateChanged)
      {
        drawNormalWarning();
        xSemaphoreTake(stateMutex, portMAX_DELAY);
        previousState = STATE_NORMAL_WARNING;
        stateStartTime = currentTime;
        xSemaphoreGive(stateMutex);
      }
      else if (currentTime - stateStartTime > 10000)
      {
        xSemaphoreTake(stateMutex, portMAX_DELAY);
        currentState = STATE_MAIN;
        xSemaphoreGive(stateMutex);
      }
      break;
    }

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void triggerWarningCountdown()
{
  xSemaphoreTake(stateMutex, portMAX_DELAY);
  currentState = STATE_WARNING_COUNTDOWN;
  countdownValue = 10;
  xSemaphoreGive(stateMutex);
}

void triggerNormalWarning()
{
  xSemaphoreTake(stateMutex, portMAX_DELAY);
  currentState = STATE_NORMAL_WARNING;
  xSemaphoreGive(stateMutex);
}

void triggerCrashScreen()
{
  xSemaphoreTake(stateMutex, portMAX_DELAY);
  currentState = STATE_CRASH;
  xSemaphoreGive(stateMutex);
}

void returnToMainScreen()
{
  xSemaphoreTake(stateMutex, portMAX_DELAY);
  currentState = STATE_MAIN;
  xSemaphoreGive(stateMutex);
}