#include "display_manager.hpp"
#include "display_manager_utils.hpp"
#include "periph/spi/ili9488/screens/screens.hpp"
#include "esp_log.h"
#include "trace/trace.h"

static const char *TAG = "display";

// Screen definition
typedef struct
{
  void (*draw)(void);
  uint32_t timeout_ms; // 0 = no auto-return
} screen_t;

// Screen draw wrappers (for uniform signature)
static void drawMain() { drawMainScreen(); }
static void drawCountdown() { drawWarningCountdown(countdownValue, true); }
static void drawCrash() { drawCrashScreen(); }
static void drawCaution() { drawCautionScreen(); }

static const screen_t screens[] = {
    [STATE_MAIN] = {drawMain, 0},
    [STATE_WARNING_COUNTDOWN] = {drawCountdown, 0}, // handled specially
    [STATE_CRASH] = {drawCrash, CRASH_SCREEN_TIMEOUT_MS},
    [STATE_NORMAL_WARNING] = {drawCaution, WARNING_SCREEN_TIMEOUT_MS},
};

// State
AppState currentState = STATE_MAIN;
AppState previousState = STATE_MAIN;
int countdownValue = INITIAL_COUNTDOWN_VALUE;
SemaphoreHandle_t stateMutex = NULL;

static unsigned long lastCountdownTime = 0;
static unsigned long stateStartTime = 0;

// Thread-safe state helpers
static void set_state(AppState state)
{
  xSemaphoreTake(stateMutex, portMAX_DELAY);
  currentState = state;
  xSemaphoreGive(stateMutex);
}

static AppState get_state(bool *changed)
{
  xSemaphoreTake(stateMutex, portMAX_DELAY);
  AppState state = currentState;
  if (changed)
    *changed = (state != previousState);
  xSemaphoreGive(stateMutex);
  return state;
}

void display_init()
{
  if (stateMutex == NULL)
  {
    stateMutex = xSemaphoreCreateMutex();
    if (stateMutex == NULL)
    {
      ESP_LOGE(TAG, "Failed to create state mutex");
      return;
    }
  }

  tft.init();
  tft.setRotation(1);
  drawMainScreen();
}

static void handleCountdownState(unsigned long currentTime)
{
  // Check for cancel button press
  if (checkCancelButton())
  {
    ESP_LOGI(TAG, "Warning cancelled by user");
    set_state(STATE_MAIN);
    return;
  }

  if (currentTime - lastCountdownTime >= COUNTDOWN_INTERVAL_MS)
  {
    xSemaphoreTake(stateMutex, portMAX_DELAY);
    countdownValue--;
    lastCountdownTime = currentTime;

    if (countdownValue <= 0)
      currentState = STATE_CRASH;
    else
      drawWarningCountdown(countdownValue, false);

    xSemaphoreGive(stateMutex);
  }
}

static void handleStateTimeout(unsigned long currentTime, uint32_t timeout)
{
  if (timeout > 0 && currentTime - stateStartTime > timeout)
    set_state(STATE_MAIN);
}

void displayTask(void *pvParameters)
{
  while (1)
  {
    TRACE_TASK_RUN(TAG);
    unsigned long currentTime = millis();

    bool stateChanged;
    AppState state = get_state(&stateChanged);

    if (stateChanged)
    {
      screens[state].draw();

      xSemaphoreTake(stateMutex, portMAX_DELAY);
      previousState = state;
      stateStartTime = currentTime;
      if (state == STATE_WARNING_COUNTDOWN)
        lastCountdownTime = currentTime;
      xSemaphoreGive(stateMutex);
    }
    else
    {
      if (state == STATE_WARNING_COUNTDOWN)
        handleCountdownState(currentTime);
      else
        handleStateTimeout(currentTime, screens[state].timeout_ms);
    }

    vTaskDelay(pdMS_TO_TICKS(DISPLAY_TASK_DELAY_MS));
  }
}

void triggerWarningCountdown()
{
  xSemaphoreTake(stateMutex, portMAX_DELAY);
  currentState = STATE_WARNING_COUNTDOWN;
  countdownValue = INITIAL_COUNTDOWN_VALUE;
  xSemaphoreGive(stateMutex);
}

void triggerNormalWarning() { set_state(STATE_NORMAL_WARNING); }
void triggerCrashScreen() { set_state(STATE_CRASH); }
void returnToMainScreen() { set_state(STATE_MAIN); }
