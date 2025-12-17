#ifndef DISPLAY_MANAGER_UTILS_HPP
#define DISPLAY_MANAGER_UTILS_HPP

#include "periph/spi/ili9488/ili9488_utils.hpp"

#define CANCEL_BTN_X ((SCREEN_WIDTH - BUTTON_WIDTH) / 2)
#define CANCEL_BTN_Y (DIALOG_BOX_Y + DIALOG_BOX_HEIGHT - 70)

bool checkButtonTouch(int16_t btnX, int16_t btnY, int16_t btnW, int16_t btnH);
bool checkCancelButton();
#endif // DISPLAY_MANAGER_UTILS_HPP
