// BRONTIDE vMix tally — Waveshare ESP32-C6-LCD-1.47 fork.
//
// PlatformIO concatenates .ino files alphabetically with the one containing
// setup()/loop() promoted to the head. Mirrors of forward-decls + extern
// state live here so any concat order compiles.

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Arduino_GFX_Library.h>
#include <FastLED.h>
#include "n_HAL.h"
#include "l_RING.h"

// Forward decls — see implementations across the other .ino files.
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

int  getBatteryLevel(void);
void renderBatteryLevel();
void drawBatteryIcon(int x, int y, int pct, uint16_t fg, uint16_t bg);
bool isCharging();

void loadSettings();
void saveWiFiPreferences(String wifi_ssid, String wifi_pass);
void resetSettings();
void increaseTally();
void resetTally();
void saveBrightness();
void printSettings();

void handle_root();
void handle_save();
void handleReconnect();
void handleScanNetwork();

void setBacklightPct(int pct);
int  brightnessPctFromVar(int b);
void ledToggle(bool val);

// ------- LCD instance -------
// Bus is owned here so that includes can declare `gfx` extern.
Arduino_DataBus *bus = new Arduino_ESP32SPI(LCD_DC, LCD_CS, LCD_SCLK, LCD_MOSI, GFX_NOT_DEFINED);
Arduino_GFX *gfx = new Arduino_ST7789(
  bus, LCD_RST, 0 /* default rotation; we rotate to landscape in setup */,
  true /* IPS */, 172, 320,
  LCD_COL_OFF, 0, LCD_COL_OFF, 0
);

// ------- Globals -------
int tnlen = 1;
Preferences preferences;

String WIFI_SSID = "";
String WIFI_PASS = "";
String VMIX_IP   = "";
String M_TALLY   = "";
int    VMIX_PORT = 8099;
int    TALLY_NR  = 1;
int    BRIGHTNESS = 12;     // 7..12 → 10..100%
int    CONN_INT  = 0;
int    MODE      = 0;
int    JUSTLIVE  = 0;

int RING_ENABLE       = 1;
int RING_BRIGHTNESS   = 40;
int RING_SHOW_PREVIEW = 1;
int RING_ONLY_LIVE    = 0;
int RING_NUM          = 1;       // 1 = onboard pixel only; bump for an external ring

String PREVIEW_URL = "";
int    PREVIEW_HZ  = 5;

int    TZ_OFFSET   = 0;          // minutes from UTC
int    SHOW_REC    = 1;
int    SHOW_STREAM = 1;
int    SHOW_CLOCK  = 1;

String semver = "3.0.0-c6";

int brightnessPctFromVar(int b) {
  if (b == 7)  return 10;
  if (b == 8)  return 20;
  if (b == 9)  return 40;
  if (b == 10) return 60;
  if (b == 11) return 80;
  return 100;
}
