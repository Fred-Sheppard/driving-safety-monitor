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
    STATE_NORMAL_WARNING
  } AppState;

  extern AppState currentState;
  extern AppState previousState;
  extern int countdownValue;
  extern SemaphoreHandle_t stateMutex;

  void display_init();
  void displayTask(void *pvParameters);

  void triggerWarningCountdown(void);
  void triggerNormalWarning(void);
  void triggerCrashScreen(void);
  void returnToMainScreen(void);

#ifdef __cplusplus
}
#endif

#endif // DISPLAY_MANAGER_HPP
