#ifndef SCREENS_HPP
#define SCREENS_HPP

void drawMainScreen();
void drawWarningCountdown(int countdown, bool fullRedraw);
void drawCrashScreen();
void drawCautionScreen();
void drawSettingsScreen();
void drawWifiScanScreen();
void drawKeyboardScreen();
void drawTouchTestScreen();

// Touch handlers for interactive screens (return true if should transition)
bool handleMainScreenTouch();
bool handleSettingsScreenTouch();
bool handleWifiScanScreenTouch();
bool handleKeyboardScreenTouch();
bool handleTouchTestScreenTouch();

// Keyboard input result
const char *getKeyboardInput();

// Selected WiFi SSID for keyboard screen
void setSelectedSsid(const char *ssid);
const char *getSelectedSsid();

#endif // SCREENS_HPP
