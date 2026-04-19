// 16-pixel NeoPixel ring driver.
//
// Dual purpose:
//   1. Tally indicator — red = LIVE, green = PREVIEW (optional).
//   2. Status ring during startup — boot chase, wifi-connecting pulse,
//      vmix-connected flash — then hands control back to the tally state.
//
// Deliberate quietness: if the vMix link drops mid-show we keep whatever
// tally colour was last displayed. No attention-grabbing disconnect
// animations — the ring must not distract the crew during a live take.
#ifndef L_RING_H
#define L_RING_H

#include <FastLED.h>

// Forward decls to settings globals (defined in a_GLOBAL.ino).
extern int RING_ENABLE;
extern int RING_BRIGHTNESS;
extern int RING_SHOW_PREVIEW;
extern int RING_ONLY_LIVE;

#define RING_DATA_PIN 26
#define RING_NUM_LEDS 16

enum RingStatus {
  RING_STATUS_BOOT,
  RING_STATUS_WIFI_CONNECTING,
  RING_STATUS_VMIX_CONNECTED,
  RING_STATUS_TALLY
};

static CRGB ringLeds[RING_NUM_LEDS];
static RingStatus ringStatus = RING_STATUS_BOOT;
static unsigned long ringStatusStart = 0;
static char ringLastTally = -1;

static inline uint8_t ringBrightness255() {
  int b = constrain(RING_BRIGHTNESS, 0, 100);
  return (uint8_t)((b * 255) / 100);
}

static inline void ringCommit() {
  if (!RING_ENABLE) {
    FastLED.clear(true);
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
    c = CRGB::Red;                               // LIVE — always on
  } else if (state == '2' && !RING_ONLY_LIVE && RING_SHOW_PREVIEW) {
    c = CRGB::Green;                             // PREVIEW — optional
  }
  fill_solid(ringLeds, RING_NUM_LEDS, c);
  ringCommit();
}

static inline void ringInit() {
  FastLED.addLeds<WS2812B, RING_DATA_PIN, GRB>(ringLeds, RING_NUM_LEDS);
  FastLED.setBrightness(ringBrightness255());
  FastLED.clear(true);
}

static inline void ringSetStatus(RingStatus s) {
  ringStatus = s;
  ringStatusStart = millis();

  if (s == RING_STATUS_VMIX_CONNECTED) {
    // 300 ms green flash, then tally takes over.
    fill_solid(ringLeds, RING_NUM_LEDS, CRGB::Green);
    ringCommit();
  } else if (s == RING_STATUS_TALLY) {
    ringRenderTally(ringLastTally);
  }
}

static inline void ringOnTally(char state) {
  // Tally wins over any status animation once it arrives.
  ringStatus = RING_STATUS_TALLY;
  ringRenderTally(state);
}

// Called from loop() — drives the boot / wifi animations.
static inline void ringTick() {
  if (!RING_ENABLE) return;

  unsigned long now = millis();
  unsigned long elapsed = now - ringStatusStart;

  if (ringStatus == RING_STATUS_BOOT) {
    // Single white pixel walking the ring once, ~750 ms per lap.
    uint8_t idx = (elapsed / 45) % RING_NUM_LEDS;
    fill_solid(ringLeds, RING_NUM_LEDS, CRGB::Black);
    ringLeds[idx] = CRGB(80, 80, 80);
    ringCommit();
  } else if (ringStatus == RING_STATUS_WIFI_CONNECTING) {
    // Slow blue breath — ~2 s period.
    uint8_t b = quadwave8((elapsed / 8) & 0xFF);
    fill_solid(ringLeds, RING_NUM_LEDS, CRGB(0, 0, b / 3 + 20));
    ringCommit();
  } else if (ringStatus == RING_STATUS_VMIX_CONNECTED) {
    // Hold the green flash for 300 ms, then transition to tally state.
    if (elapsed > 300) {
      ringStatus = RING_STATUS_TALLY;
      ringRenderTally(ringLastTally);
    }
  }
  // RING_STATUS_TALLY: static colour, no per-tick work needed.
}

#endif
