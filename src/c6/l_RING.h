// NeoPixel status / tally ring driver.
//
// Dual purpose:
//   1. Tally indicator — red = LIVE, green = PREVIEW (optional).
//   2. Status ring during startup — boot chase, wifi-connecting pulse,
//      vmix-connected flash — then hands control back to tally state.
//
// On the C6 board the onboard WS2812 (GPIO8) doubles as a status pixel.
// An external ring can be daisy-chained off the same GPIO8 line; the
// number of pixels is configurable at runtime via NVS.
//
// Deliberate quietness: if the vMix link drops mid-show we keep whatever
// tally colour was last displayed — no attention-grabbing animations.
#ifndef L_RING_H
#define L_RING_H

#include <FastLED.h>
#include "n_HAL.h"

extern int RING_ENABLE;
extern int RING_BRIGHTNESS;
extern int RING_SHOW_PREVIEW;
extern int RING_ONLY_LIVE;
extern int RING_NUM;

enum RingStatus {
  RING_STATUS_BOOT,
  RING_STATUS_WIFI_CONNECTING,
  RING_STATUS_VMIX_CONNECTED,
  RING_STATUS_TALLY
};

#define RING_MAX_LEDS 64
static CRGB ringLeds[RING_MAX_LEDS];
static RingStatus ringStatus = RING_STATUS_BOOT;
static unsigned long ringStatusStart = 0;
static char ringLastTally = -1;

static inline uint8_t ringBrightness255() {
  int b = constrain(RING_BRIGHTNESS, 0, 100);
  return (uint8_t)((b * 255) / 100);
}

static inline int ringActiveCount() {
  return constrain(RING_NUM, 1, RING_MAX_LEDS);
}

static inline void ringCommit() {
  if (!RING_ENABLE) {
    fill_solid(ringLeds, RING_MAX_LEDS, CRGB::Black);
    FastLED.show();
    return;
  }
  FastLED.setBrightness(ringBrightness255());
  FastLED.show();
}

static inline void ringRenderTally(char state) {
  ringLastTally = state;
  if (!RING_ENABLE) { FastLED.clear(true); return; }

  CRGB c = CRGB::Black;
  if (state == '1') {
    c = CRGB::Red;
  } else if (state == '2' && !RING_ONLY_LIVE && RING_SHOW_PREVIEW) {
    c = CRGB::Green;
  }
  fill_solid(ringLeds, ringActiveCount(), c);
  ringCommit();
}

static inline void ringInit() {
  FastLED.addLeds<WS2812B, WS2812_PIN, GRB>(ringLeds, RING_MAX_LEDS);
  FastLED.setBrightness(ringBrightness255());
  FastLED.clear(true);
}

static inline void ringSetStatus(RingStatus s) {
  ringStatus = s;
  ringStatusStart = millis();
  if (s == RING_STATUS_VMIX_CONNECTED) {
    fill_solid(ringLeds, ringActiveCount(), CRGB::Green);
    ringCommit();
  } else if (s == RING_STATUS_TALLY) {
    ringRenderTally(ringLastTally);
  }
}

static inline void ringOnTally(char state) {
  ringStatus = RING_STATUS_TALLY;
  ringRenderTally(state);
}

static inline void ringTick() {
  if (!RING_ENABLE) return;

  unsigned long elapsed = millis() - ringStatusStart;
  int n = ringActiveCount();

  if (ringStatus == RING_STATUS_BOOT) {
    uint8_t idx = (elapsed / 45) % (n > 0 ? n : 1);
    fill_solid(ringLeds, n, CRGB::Black);
    ringLeds[idx] = CRGB(80, 80, 80);
    ringCommit();
  } else if (ringStatus == RING_STATUS_WIFI_CONNECTING) {
    uint8_t b = quadwave8((elapsed / 8) & 0xFF);
    fill_solid(ringLeds, n, CRGB(0, 0, b / 3 + 20));
    ringCommit();
  } else if (ringStatus == RING_STATUS_VMIX_CONNECTED) {
    if (elapsed > 300) {
      ringStatus = RING_STATUS_TALLY;
      ringRenderTally(ringLastTally);
    }
  }
}

#endif
