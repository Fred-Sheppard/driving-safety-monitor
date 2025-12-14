#include "display_manager_utils.hpp"

bool checkButtonTouch(int16_t btnX, int16_t btnY, int16_t btnW, int16_t btnH)
{
  int32_t x, y;
  if (tft.getTouch(&x, &y))
  {
    if (x >= btnX && x < btnX + btnW &&
        y >= btnY && y < btnY + btnH)
    {
      return true;
    }
  }
  return false;
}

bool checkCancelButton()
{
  return checkButtonTouch(CANCEL_BTN_X, CANCEL_BTN_Y, BUTTON_WIDTH, BUTTON_HEIGHT);
}

bool checkDismissButton()
{
  return checkButtonTouch(DISMISS_BTN_X, DISMISS_BTN_Y, DISMISS_BTN_WIDTH, BUTTON_HEIGHT);
}
