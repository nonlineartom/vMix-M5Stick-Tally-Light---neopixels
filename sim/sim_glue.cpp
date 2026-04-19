// Sim-only glue for the Wokwi simulator.
//
// M5.Lcd is a direct reference to a TFT_eSPI instance driving the Wokwi
// ILI9341 (240x320). The sketch's drawing code runs unchanged; layout is
// simply wider/taller than on a real 80x160 StickC.
#ifdef SIM_WOKWI

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "M5StickC.h"

static TFT_eSPI tft_device;

// Charging simulator — flip to exercise the bolt overlay.
bool SIM_CHARGING = false;

SimM5 M5(tft_device);

void simFlush() {
  // Direct-draw mode: nothing to push, drawings go straight to hardware.
}

void SimM5::begin() {
  tft_device.init();
  tft_device.setRotation(1);                 // 320x240 landscape
  tft_device.fillScreen(TFT_BLACK);
  tft_device.setTextColor(TFT_WHITE, TFT_BLACK);
}

#endif
