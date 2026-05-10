// Live preview thumbnail.
//
// Polls a tiny companion HTTP proxy on the vMix host (see
// tools/vmix-snapshot-proxy/) for low-fps JPEG snapshots of the
// configured input and blits them into the middle band of the LCD.
//
// Hard rule: tally state must never lag because of this. The fetch is
// non-blocking-ish — we use a single short-timeout HTTPClient call per
// loop tick, gated by the configured poll interval, and yield back to
// the main loop after each frame. Failed fetches decay the poll rate
// to 5 s and surface a small "preview offline" indicator.
#ifndef M_PREVIEW_H
#define M_PREVIEW_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <JPEGDEC.h>
#include "n_HAL.h"

extern String PREVIEW_URL;     // empty = feature disabled
extern int    PREVIEW_HZ;      // target poll rate, 1..15
extern bool   apEnabled;
extern WiFiClient client;      // vMix TCP socket — we never touch it from here

// Preview window — tucked between the top tally band and the bottom
// status strip. 320x110 keeps decode under ~50 ms on the C6.
#define PREVIEW_X 0
#define PREVIEW_Y 60
#define PREVIEW_W 320
#define PREVIEW_H 110

static JPEGDEC      _jpeg;
static unsigned long _previewLastFetch = 0;
static unsigned long _previewBackoffUntil = 0;
static bool         _previewOk = false;
static bool         _previewActive = false;
static int16_t      _previewDx = 0;   // x offset to centre the decoded JPEG
static int16_t      _previewDy = 0;

static int _jpegDraw(JPEGDRAW *d) {
  // Clip to the preview window.
  int16_t x = _previewDx + d->x;
  int16_t y = _previewDy + d->y;
  if (x >= PREVIEW_X + PREVIEW_W || y >= PREVIEW_Y + PREVIEW_H) return 1;
  int16_t w = d->iWidth;
  int16_t h = d->iHeight;
  if (x + w > PREVIEW_X + PREVIEW_W) w = (PREVIEW_X + PREVIEW_W) - x;
  if (y + h > PREVIEW_Y + PREVIEW_H) h = (PREVIEW_Y + PREVIEW_H) - y;
  if (w <= 0 || h <= 0) return 1;
  gfx->draw16bitRGBBitmap(x, y, d->pPixels, w, h);
  return 1;
}

static inline bool previewEnabled() {
  return PREVIEW_URL.length() > 0 && WiFi.status() == WL_CONNECTED && !apEnabled;
}

// Render the bordered "frame" once when preview is first activated, plus
// the offline state if we ever lose the proxy.
static inline void previewDrawChrome(bool online) {
  gfx->fillRect(PREVIEW_X, PREVIEW_Y, PREVIEW_W, PREVIEW_H, BLACK);
  gfx->drawRect(PREVIEW_X, PREVIEW_Y, PREVIEW_W, PREVIEW_H, online ? 0x4A49 : 0x8410);
  if (!online) {
    const char *msg = "preview offline";
    int16_t w = textPxWidth(msg, 1);
    gfx->setTextSize(1);
    gfx->setTextColor(0xC618, BLACK);
    gfx->setCursor(PREVIEW_X + (PREVIEW_W - w) / 2, PREVIEW_Y + PREVIEW_H / 2 - 4);
    gfx->print(msg);
  }
}

static inline void previewActivate() {
  _previewActive = true;
  previewDrawChrome(false);
}

static inline void previewDeactivate() {
  _previewActive = false;
}

static inline void previewTick() {
  if (!previewEnabled()) {
    if (_previewActive) previewDeactivate();
    return;
  }
  if (!_previewActive) previewActivate();

  unsigned long now = millis();
  if (now < _previewBackoffUntil) return;

  int hz = constrain(PREVIEW_HZ, 1, 15);
  unsigned long period = 1000UL / hz;
  if (now - _previewLastFetch < period) return;
  _previewLastFetch = now;

  HTTPClient http;
  http.setTimeout(600);
  http.setReuse(false);
  if (!http.begin(PREVIEW_URL)) {
    _previewBackoffUntil = now + 5000;
    if (_previewOk) { _previewOk = false; previewDrawChrome(false); }
    return;
  }
  int code = http.GET();
  if (code != 200) {
    http.end();
    _previewBackoffUntil = now + 5000;
    if (_previewOk) { _previewOk = false; previewDrawChrome(false); }
    return;
  }

  // Read the full JPEG into a heap buffer. Small frames only — we cap at
  // 32 KB which is plenty for a 320x110 quality-70 JPEG and well under
  // the C6's free heap.
  int len = http.getSize();
  if (len <= 0 || len > 32 * 1024) {
    http.end();
    _previewBackoffUntil = now + 2000;
    return;
  }
  uint8_t *buf = (uint8_t *)malloc(len);
  if (!buf) { http.end(); _previewBackoffUntil = now + 2000; return; }
  WiFiClient *stream = http.getStreamPtr();
  int read = 0;
  unsigned long deadline = now + 700;
  while (read < len && millis() < deadline) {
    int n = stream->read(buf + read, len - read);
    if (n > 0) read += n;
    else delay(1);
  }
  http.end();

  if (read != len) {
    free(buf);
    _previewBackoffUntil = millis() + 1000;
    return;
  }

  if (_jpeg.openRAM(buf, len, _jpegDraw)) {
    int iw = _jpeg.getWidth();
    int ih = _jpeg.getHeight();
    _previewDx = PREVIEW_X + (PREVIEW_W - iw) / 2;
    _previewDy = PREVIEW_Y + (PREVIEW_H - ih) / 2;
    _jpeg.decode(0, 0, 0);
    _jpeg.close();
    if (!_previewOk) {
      _previewOk = true;
      gfx->drawRect(PREVIEW_X, PREVIEW_Y, PREVIEW_W, PREVIEW_H, 0x4A49);
    }
  } else {
    _previewBackoffUntil = millis() + 1000;
  }
  free(buf);
}

#endif
