#ifndef ILI9488_UTILS_HPP
#define ILI9488_UTILS_HPP

#include "lgfx_setup.hpp"
#include <stdint.h>
extern LGFX_ILI9488 tft;

extern uint16_t GRASS_GREEN;
extern uint16_t DARK_GREEN;
extern uint16_t ROAD_GREY;
extern uint16_t ROAD_DARK;
extern uint16_t TREE_GREEN;
extern uint16_t BROWN;
extern uint16_t CAR_BLUE;

void initColors();
unsigned long millis();
void delay(unsigned long ms);
int rnd(int min, int max);

void drawTree(int x, int y, int size, uint16_t trunk, uint16_t foliage1, uint16_t foliage2);
void drawRoadSegment(uint16_t color, int width);
void drawCenterLine();

#endif // ILI9488_UTILS_HPP