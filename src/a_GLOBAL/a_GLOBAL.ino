// Default to the Plus2 build; overridable from platformio.ini / -DC_PLUS=N.
// 0 = original M5StickC, 1 = M5StickC-Plus, 2 = M5StickC-Plus2.
#ifndef C_PLUS
  #define C_PLUS 2
#endif

#if C_PLUS == 2
  #include <M5Unified.h>
#elif C_PLUS == 1
  #include <M5StickCPlus.h>
#else
  #include <M5StickC.h>
#endif

#include <WiFi.h>
#include <PinButton.h>
#include <WebServer.h>
#include <Preferences.h>
#include "k_PLUGINMANAGER.h"
#include "l_RING.h"

#if C_PLUS == 2
  #define LED_BUILTIN 19   // Plus2: internal LED on GPIO19
#else
  #define LED_BUILTIN 10   // Plus/older StickC Plus
#endif

// Button GPIOs. Real M5StickC wires BtnA to G37 and BtnB to G39. The Wokwi
// DevKitC model doesn't break G37 out, so in sim we remap the side button
// to G35 (same "input-only, no pull-up" class of pin).
#ifdef SIM_WOKWI
  #define BTN_M5_PIN     35
  #define BTN_ACTION_PIN 39
#else
  #define BTN_M5_PIN     37
  #define BTN_ACTION_PIN 39
#endif

// -------------------------------------------------
// Function prototypes (Arduino IDE 2.x fix)
// -------------------------------------------------
void renderCurrentScreen();
boolean connectTovMix(bool recursive);
void singleReconnect();
boolean retryConnectionvMix(int tryCount);

void showNetworkScreen();
void showTallyNum();
void showBrightnessScreen();
void showTallyScreen();
void showAPScreen();
void showStatus();

void updateBrightnessVar();
void updateBrightness();

void startWiFi();
void startLocalWiFi();
void startServer();
void drawWiFiIcon(int x, int y);

void handleData(String data);
void noConnectionTovMix();

void resetScreen();
void cls();

int lcdCoordX(int x);
int lcdCoordY(int y);

int getBatteryLevel(void);
void renderBatteryLevel();
void drawBatteryIcon(int x, int y, int pct, uint16_t fg, uint16_t bg);
bool isCharging();

// --- Brightness helper (for Plus2 UI + setBrightness mapping) ---
int brightnessPctFromVar(int b) {
  if (b == 7) return 10;   // minimum (not 0, otherwise black screen)
  if (b == 8) return 20;
  if (b == 9) return 40;
  if (b == 10) return 60;
  if (b == 11) return 80;
  return 100;
}

int tnlen = 1; //LET THIS BE

Preferences preferences;

//DON'T CHANGE THESE VARIABLES, YOU CAN CHANGE THEM IN THE WEB UI
String WIFI_SSID = "";
String WIFI_PASS = "";
String VMIX_IP = "";
String M_TALLY = "";
int VMIX_PORT = 8099; //USES THE TCP API PORT, THIS IS FIXED IN VMIX
int TALLY_NR = 1;
int BRIGHTNESS = 12; //100%
int CONN_INT = 0;
int MODE = 0; //0 for words like SAFE, PRE and LIVE. 1 for numbers with changing background
int JUSTLIVE = 0; //When 1, SAFE and PRE are not used. Just the LIVE screen

// NeoPixel status / tally ring (see l_RING.h).
int RING_ENABLE = 1;
int RING_BRIGHTNESS = 40;      // 0..100 → mapped to FastLED 0..255
int RING_SHOW_PREVIEW = 1;     // show green for PREVIEW
int RING_ONLY_LIVE = 0;        // 1 → only light up on LIVE

String semver = "2.5.0";
