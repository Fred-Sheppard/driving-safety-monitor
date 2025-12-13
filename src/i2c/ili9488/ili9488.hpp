#ifndef ILI9488_HPP
#define ILI9488_HPP

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

  void tft_init();
  void displayTask(void *pvParameters);

  void triggerWarningCountdown(void);
  void triggerNormalWarning(void);
  void triggerCrashScreen(void);
  void returnToMainScreen(void);

#ifdef __cplusplus
}
#endif

#endif // ILI9488_HPP
