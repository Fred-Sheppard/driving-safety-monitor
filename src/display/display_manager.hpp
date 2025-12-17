#ifndef DISPLAY_MANAGER_HPP
#define DISPLAY_MANAGER_HPP

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#ifdef __cplusplus
extern "C"
{
#endif

  typedef enum
  {
    STATE_MAIN,
    STATE_WARNING_COUNTDOWN,
    STATE_CRASH,
    STATE_NORMAL_WARNING,
    STATE_SETTINGS,
    STATE_WIFI_SCAN,
    STATE_KEYBOARD,
    STATE_TOUCH_TEST
  } AppState;

  extern AppState currentState;
  extern AppState previousState;
  extern int countdownValue;
  extern SemaphoreHandle_t stateMutex;

  void display_init();
  void displayTask(void *pvParameters);

  void triggerWarningCountdown(const char *message);
  void triggerNormalWarning(const char *message);
  void triggerCrashScreen(void);
  void returnToMainScreen(void);
  void triggerSettingsScreen(void);
  void triggerWifiScanScreen(void);
  void triggerKeyboardScreen(const char *ssid);
  void triggerTouchTestScreen(void);

#ifdef __cplusplus
}
#endif

#endif // DISPLAY_MANAGER_HPP
