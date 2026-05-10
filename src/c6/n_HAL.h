// HAL for the Waveshare ESP32-C6-LCD-1.47 board.
// Owns pin defs, colour aliases, small graphics helpers, and the
// cross-file forward declarations that the .ino concat order needs
// (PlatformIO puts setup()/loop()'s file at the top of the merged TU,
// so c_MAIN.ino's references to functions defined elsewhere need decls
// reachable from c_MAIN's includes — i.e. here).
#ifndef N_HAL_H
#define N_HAL_H

#include <Arduino.h>
#include <Arduino_GFX_Library.h>

// ------- Pin map (Waveshare ESP32-C6-LCD-1.47) -------
#define LCD_MOSI    6
#define LCD_SCLK    7
#define LCD_CS      14
#define LCD_DC      15
#define LCD_RST     21
#define LCD_BL      22

#define LCD_W       320
#define LCD_H       172
#define LCD_COL_OFF 34   // ST7789 panel is 240-wide; 172-wide window starts at col 34

#define WS2812_PIN  8
#define WS2812_NUM  1

#define SD_CS       4
#define SD_MISO     5

#define BOOT_BTN    9    // active low
#define BATT_ADC    0    // GPIO0 with on-board divider — confirm against schematic

// ------- Colour aliases (RGB565) -------
// Arduino_GFX defines these in older versions; v1.6.x dropped the global
// macros, so re-declare unconditionally here.
#undef BLACK
#undef WHITE
#undef RED
#undef GREEN
#undef BLUE
#undef YELLOW
#define BLACK   0x0000
#define WHITE   0xFFFF
#define RED     0xF800
#define GREEN   0x07E0
#define BLUE    0x001F
#define YELLOW  0xFFE0
#ifndef TFT_BLACK
  #define TFT_BLACK BLACK
#endif

// ------- LCD instance (defined in a_GLOBAL.ino) -------
extern Arduino_GFX *gfx;

// ------- Small helpers -------
// Cheap text-width approximation for the built-in 6x8 font. Arduino_GFX
// also exposes getTextBounds() but for the fixed-pitch built-in font this
// is faster and avoids string allocation.
static inline int16_t textPxWidth(const String &s, uint8_t size) {
  return (int16_t)s.length() * 6 * (int16_t)size;
}
static inline int16_t textPxWidth(const char *s, uint8_t size) {
  return (int16_t)strlen(s) * 6 * (int16_t)size;
}

// ------- Cross-file forward declarations -------
// All functions referenced by c_MAIN.ino but defined in other .ino files.
// Without these, the c_MAIN-first concat order won't compile.
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

#endif
