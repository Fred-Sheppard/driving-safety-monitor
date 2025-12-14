#include "map_elements.hpp"
#include "../ili9488_utils.hpp"

static void drawTree(int x, int y, int size)
{
  int trunkWidth = size / 5;
  tft.fillRect(x - trunkWidth / 2, y, trunkWidth, size / 2, COLOR_BROWN);

  int radius = size / 2;
  tft.fillCircle(x, y, radius, COLOR_TREE_GREEN);
  tft.fillCircle(x - radius / 2, y - radius / 3, radius / 2 + 2, COLOR_DARK_GREEN);
  tft.fillCircle(x + radius / 2, y - radius / 3, radius / 2 + 2, COLOR_DARK_GREEN);
}

static void drawRoadSegment(uint16_t color, int width)
{
  int halfWidth = width / 2;

  for (int y = 0; y < 60; y += 2)
    tft.fillCircle(120, y, halfWidth, color);

  for (int y = 60; y < 120; y += 2)
    tft.fillCircle(120 + (int)((y - 60) / 60.0 * 60), y, halfWidth, color);

  for (int y = 120; y < 220; y += 2)
    tft.fillCircle(180 - (int)((y - 120) / 100.0 * 120), y, halfWidth, color);

  for (int y = 220; y < SCREEN_HEIGHT; y += 2)
    tft.fillCircle(60 + (int)((y - 220) / 100.0 * 60), y, halfWidth, color);
}

static void drawCenterLine()
{
  const int dashLen = 8, gapLen = 8;
  int counter = 0;
  bool draw = true;

  auto drawDash = [&](int x, int y)
  {
    if (draw && counter < dashLen)
    {
      tft.drawPixel(x, y, TFT_YELLOW);
      tft.drawPixel(x + 1, y, TFT_YELLOW);
    }
    if (++counter >= dashLen + gapLen)
      counter = 0;
    draw = (counter < dashLen);
  };

  for (int y = 0; y < 60; y++)
    drawDash(120, y);

  counter = 0;
  draw = true;
  for (int y = 60; y < 120; y++)
    drawDash(120 + (int)((y - 60) / 60.0 * 60), y);

  counter = 0;
  draw = true;
  for (int y = 120; y < 220; y++)
    drawDash(180 - (int)((y - 120) / 100.0 * 120), y);

  counter = 0;
  draw = true;
  for (int y = 220; y < SCREEN_HEIGHT; y++)
    drawDash(60 + (int)((y - 220) / 100.0 * 60), y);
}

void drawRoad()
{
  drawRoadSegment(COLOR_ROAD_DARK, 35);
  drawRoadSegment(COLOR_ROAD_GREY, 30);
  drawCenterLine();
}

void drawTrees()
{
  for (int i = 0; i < 25; i++)
  {
    int x = rnd(30, SCREEN_WIDTH + 1);
    int y = rnd(HEADER_HEIGHT, SCREEN_HEIGHT - FOOTER_HEIGHT + 1);
    int size = rnd(10, 22);

    bool tooClose = false;
    if (y < 60 && abs(x - 120) < 40)
      tooClose = true;
    else if (y < 120 && abs(x - (120 + (int)((y - 60) / 60.0 * 60))) < 40)
      tooClose = true;
    else if (y < 220 && abs(x - (180 - (int)((y - 120) / 100.0 * 120))) < 40)
      tooClose = true;
    else if (abs(x - (60 + (int)((y - 220) / 100.0 * 60))) < 40)
      tooClose = true;

    if (!tooClose)
      drawTree(x, y, size);
  }
}

void drawCar()
{
  const int carX = 180, carY = 120;
  tft.fillRect(carX - 8, carY - 10, 16, 20, COLOR_CAR_BLUE);
  tft.fillRect(carX - 6, carY - 6, 12, 10, COLOR_CAR_DARK_BLUE);
  tft.fillRect(carX - 4, carY - 4, 8, 6, COLOR_CAR_WINDOW);
}
