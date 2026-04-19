// Shim that stands in for the real M5StickC.h library when building for Wokwi.
// -I sim in the wokwi env puts this ahead of libs/M5StickC/ on the include path,
// so the sketch compiles against these stubs instead of the hardware driver.
//
// What this provides:
//   * M5 global with .Lcd (an offscreen TFT_eSprite at the real stick's
//     native 160x80 landscape resolution), .Axp, .IMU, .begin()
//   * Color aliases (WHITE, BLACK, RED, GREEN, ...) that the M5 library defines
//   * simFlush() — push the sprite to the ILI9341 on the Wokwi ESP32
//
// The .Lcd reference has the same surface as the real M5Display / TFT_eSPI,
// so none of the sketch's drawing calls need to change.
#ifndef SIM_M5STICKC_H
#define SIM_M5STICKC_H

#include <Arduino.h>
#include <TFT_eSPI.h>

// ---- color aliases the real M5 library provides ----
#ifndef WHITE
#define WHITE  TFT_WHITE
#endif
#ifndef BLACK
#define BLACK  TFT_BLACK
#endif
#ifndef RED
#define RED    TFT_RED
#endif
#ifndef GREEN
#define GREEN  TFT_GREEN
#endif
#ifndef BLUE
#define BLUE   TFT_BLUE
#endif
#ifndef YELLOW
#define YELLOW TFT_YELLOW
#endif
#ifndef ORANGE
#define ORANGE TFT_ORANGE
#endif

// Real StickC panel is ST7735 with 80x160 portrait native — the sketch's
// setRotation(3) calls flip it to 160x80 landscape at runtime.

// ---- AXP192 stub ----
// Register 0x00, bit 6 = "ACIN present" — we treat it as "charging".
// You can toggle SIM_CHARGING at runtime from anywhere to exercise the bolt.
extern bool SIM_CHARGING;

class SimAxp {
public:
  uint16_t GetVbatData() { return 3700; } // → ~65% via the sketch's formula
  void ScreenBreath(uint8_t) {}
  float GetBatCurrent() { return SIM_CHARGING ? 100.0f : 0.0f; }
};

// ---- IMU stub: fixed landscape orientation, no accelerometer events ----
class SimIMU {
public:
  void Init() {}
  void getAccelData(float* x, float* y, float* z) {
    if (x) *x = -1.0f;  // matches screenRotation == 3 branch in c_MAIN.ino
    if (y) *y =  0.0f;
    if (z) *z =  0.0f;
  }
};

// ---- The M5 container ----
class SimM5 {
public:
  TFT_eSPI& Lcd;     // direct handle to the Wokwi ILI9341 — layout scales up to 320x240
  SimAxp Axp;
  SimIMU IMU;

  SimM5(TFT_eSPI& lcd) : Lcd(lcd) {}

  void begin();
};

// Global, defined in sim_glue.cpp.
extern SimM5 M5;

// Flush the sprite to the real ILI9341 on the Wokwi ESP32. Called by a
// background FreeRTOS task at ~30 Hz; also callable directly.
void simFlush();

#endif
