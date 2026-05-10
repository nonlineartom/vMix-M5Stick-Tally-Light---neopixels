// setup()/loop() + battery + button + backlight + small render helpers.
//
// PlatformIO promotes the .ino containing setup()/loop() to the head of the
// concatenated translation unit; mirror the include set so other orderings
// (like Arduino IDE) compile too.

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <time.h>
#include <Arduino_GFX_Library.h>
#include <FastLED.h>
#include "n_HAL.h"
#include "l_RING.h"
#include "m_PREVIEW.h"

extern Arduino_GFX *gfx;
extern Preferences preferences;
extern int tnlen;
extern String WIFI_SSID, WIFI_PASS, VMIX_IP, M_TALLY, semver, PREVIEW_URL;
extern int VMIX_PORT, TALLY_NR, BRIGHTNESS, CONN_INT, MODE, JUSTLIVE;
extern int RING_ENABLE, RING_BRIGHTNESS, RING_SHOW_PREVIEW, RING_ONLY_LIVE, RING_NUM;
extern int PREVIEW_HZ, TZ_OFFSET, SHOW_REC, SHOW_STREAM, SHOW_CLOCK;
extern int brightnessPctFromVar(int b);

WiFiClient client;
WebServer  server(80);

char  currentState  = -1;
char  screen        = 0;
bool  connectedTovMix = false;
bool  apEnabled     = false;
bool  started       = false;
int   status        = WL_IDLE_STATUS;

int   interval         = 5000;
unsigned long lastCheck       = 0;
unsigned long lastConnCheck   = 0;
unsigned long lastBattCheck   = 0;
unsigned long sigStrengthChk  = 0;
long  rssi      = -100;
String rssiLabel = "";

// REC/STREAM state — populated from ACTS events in d_VMIX.ino.
extern bool rec, stm;
extern unsigned long recStart, stmStart;

int  battShown        = -1;
unsigned long lastBattUiUpdate = 0;
char lastBattState    = -1;
int  lastBattDrawn    = -1;

// Single-button input via BOOT (active low).
unsigned long _btnDownAt = 0;
bool          _btnPrev   = false;
bool          _btnLongFired = false;
bool          _btnShortPending = false;

bool buttonShortClick() {
  if (_btnShortPending) { _btnShortPending = false; return true; }
  return false;
}
bool buttonLongClick() {
  bool now = (digitalRead(BOOT_BTN) == LOW);
  unsigned long t = millis();
  if (now && !_btnPrev) {
    _btnDownAt = t; _btnLongFired = false;
  } else if (!now && _btnPrev) {
    if (!_btnLongFired && (t - _btnDownAt) > 30 && (t - _btnDownAt) < 800) {
      _btnShortPending = true;
    }
  } else if (now && _btnPrev) {
    if (!_btnLongFired && (t - _btnDownAt) > 1500) {
      _btnLongFired = true;
      _btnPrev = now;
      return true;
    }
  }
  _btnPrev = now;
  return false;
}

uint16_t getBatteryBgColor() {
  if (currentState == '1') return RED;
  if (currentState == '2') return GREEN;
  return BLACK;
}
uint16_t getBatteryTextColor() {
  if (currentState == '2') return BLACK;
  return WHITE;
}

bool isCharging() {
  // The Waveshare board doesn't break out a Vbus-detect line on a public
  // GPIO; treat "USB connected" as "battery reads above 4.15 V". Good
  // enough for the on-screen indicator.
  return analogReadMilliVolts(BATT_ADC) * 2 > 4150;
}

int getBatteryLevel(void) {
  // BATT_ADC reads half of Vbat through the on-board divider.
  uint32_t mv = analogReadMilliVolts(BATT_ADC);
  float vbat = (mv * 2) / 1000.0f;
  int pct = (int)(100.0f * (vbat - 3.30f) / (4.15f - 3.30f));
  return constrain(pct, 0, 100);
}

void setBacklightPct(int pct) {
  pct = constrain(pct, 0, 100);
  static bool _attached = false;
  if (!_attached) {
    ledcAttach(LCD_BL, 5000, 8);
    _attached = true;
  }
  ledcWrite(LCD_BL, (pct * 255) / 100);
}

void ledToggle(bool /*val*/) {
  // No discrete user LED on this board beyond the WS2812 pixel; no-op.
}

void drawBatteryIcon(int x, int y, int pct, uint16_t fg, uint16_t bg) {
  pct = constrain(pct, 0, 100);
  int w = 28, h = 14;
  gfx->fillRect(x - 2, y - 2, w + 6, h + 4, bg);
  gfx->drawRect(x, y, w, h, fg);
  gfx->fillRect(x + w, y + h / 4, 3, h / 2, fg);
  gfx->fillRect(x + 2, y + 2, w - 4, h - 4, bg);
  int fillW = map(pct, 0, 100, 0, w - 4);
  gfx->fillRect(x + 2, y + 2, fillW, h - 4, fg);
  if (isCharging()) {
    int cx = x + w / 2, cy = y + h / 2;
    gfx->fillTriangle(cx - 3, cy - 5, cx + 3, cy - 1, cx - 1, cy - 1, YELLOW);
    gfx->fillTriangle(cx + 1, cy + 1, cx - 3, cy + 5, cx + 3, cy + 1, YELLOW);
  }
}

void cls() {
  gfx->fillScreen(BLACK);
  gfx->setCursor(0, 0);
}

void resetScreen() {
  cls();
  gfx->setTextSize(1);
  gfx->setTextColor(WHITE, BLACK);
}

void start();
void renderCurrentScreen();

void setup() {
  Serial.begin(115200);

  // LCD up first so we can show a splash before WiFi/NVS work.
  gfx->begin(40000000UL);     // 40 MHz SPI
  gfx->setRotation(1);        // landscape 320x172
  gfx->fillScreen(BLACK);
  setBacklightPct(80);

  pinMode(BOOT_BTN, INPUT_PULLUP);
  analogReadResolution(12);
  pinMode(BATT_ADC, INPUT);

  loadSettings();

  ringInit();
  ringSetStatus(RING_STATUS_BOOT);

  Serial.printf("BRONTIDE tally v%s\n", semver.c_str());
}

void loop() {
  server.handleClient();

  if (buttonShortClick()) {
    if      (screen == 0) showNetworkScreen();
    else if (screen == 1) showTallyNum();
    else if (screen == 2) showBrightnessScreen();
    else if (screen == 3) showTallyScreen();
    else                  showTallyScreen();
  }
  if (buttonLongClick()) {
    if (!client.connected() && screen == 1) {
      resetSettings();
    } else if (screen == 0) {
      // Hold-on-tally to force reconnect.
      connectTovMix(false);
    } else if (screen == 3) {
      updateBrightnessVar();
    }
  }

  if (!started) { started = true; start(); }

  while (client.available()) {
    server.handleClient();
    String data = client.readStringUntil('\n');
    handleData(data);
  }

  if (screen == 0) {
    if (millis() - lastBattCheck > 5000) {
      lastBattCheck = millis();
      renderBatteryLevel();
      drawWiFiIcon(gfx->width() - 30, 2);
    }
    showStatus();              // refresh REC/STM/clock once a second-ish
    previewTick();             // non-blocking JPEG poll
  }

  ringTick();

  if (screen == 1 && millis() > sigStrengthChk + interval) {
    rssi = WiFi.RSSI();
    sigStrengthChk = millis();
    if      (rssi > -67) rssiLabel = "strong";
    else if (rssi > -85) rssiLabel = "average";
    else                 rssiLabel = "poor";
    showNetworkScreen();
  }

  if (CONN_INT != 0 && !client.connected() && !apEnabled &&
      millis() > lastConnCheck + (CONN_INT * 1000UL)) {
    client.stop();
    singleReconnect();
  }

  if (!client.connected() && !apEnabled && millis() > lastCheck + interval) {
    client.stop();
    lastCheck = millis();
    noConnectionTovMix();
  }
}

void start() {
  cls();
  loadSettings();

  String prod = "BRONTIDE.MEDIA";
  gfx->setTextSize(2);
  gfx->setTextColor(WHITE, BLACK);
  gfx->setCursor(8, 8);
  gfx->print("v"); gfx->println(semver);
  gfx->setTextSize(3);
  gfx->setCursor((gfx->width() - textPxWidth(prod, 3)) / 2, gfx->height() / 2 - 12);
  gfx->println(prod);

  unsigned long splashUntil = millis() + 2000;
  while (millis() < splashUntil) { ringTick(); delay(30); }
  startWiFi();
}

void renderBatteryLevel() {
  int battLvl = getBatteryLevel();
  battLvl = constrain(battLvl, 0, 100);
  if (battShown < 0) battShown = battLvl;
  int diff = battLvl - battShown;
  if (abs(diff) >= 3) battShown += (diff > 0) ? 1 : -1;

  bool stateChanged = (currentState != lastBattState);
  bool valueChanged = (battShown   != lastBattDrawn);
  if (stateChanged) { lastBattState = currentState; lastBattUiUpdate = 0; }
  if (!stateChanged && !valueChanged && (millis() - lastBattUiUpdate < 2000)) return;
  lastBattUiUpdate = millis();
  lastBattDrawn = battShown;

  uint16_t bg = getBatteryBgColor();
  uint16_t fg = getBatteryTextColor();

  gfx->fillRect(0, 0, 140, 26, bg);
  drawBatteryIcon(4, 8, battShown, fg, bg);
  gfx->setTextSize(2);
  gfx->setTextColor(fg, bg);
  gfx->setCursor(42, 4);
  gfx->print(battShown); gfx->print("% ");
}

void renderCurrentScreen() {
  if (!client.connected()) return;
  if      (screen == 0) showTallyScreen();
  else if (screen == 1) showNetworkScreen();
  else if (screen == 2) showTallyNum();
  else if (screen == 4) showAPScreen();
}

void drawWiFiIcon(int x, int y) {
  uint16_t bg = (currentState == '1') ? RED : (currentState == '2') ? GREEN : BLACK;
  uint16_t fg = (currentState == '2') ? BLACK : WHITE;

  int barW = 5, gap = 3, h1 = 8, h2 = 12, h3 = 16;
  gfx->fillRect(x - 2, y - 2, 3 * (barW + gap) + 8, h3 + 6, bg);

  if (!client.connected() && !apEnabled) {
    gfx->drawLine(x, y, x + 18, y + 18, fg);
    gfx->drawLine(x + 18, y, x, y + 18, fg);
    return;
  }
  if (apEnabled) {
    gfx->drawRect(x + 6, y + 2, 10, 14, fg);
    gfx->fillRect(x + 6, y + 18, 10, 3, fg);
    return;
  }

  int rssiNow = WiFi.RSSI();
  int bars = 1;
  if      (rssiNow > -67) bars = 3;
  else if (rssiNow > -80) bars = 2;
  int baseY = y + h3;
  if (bars >= 1) gfx->fillRect(x + 0 * (barW + gap), baseY - h1, barW, h1, fg);
  if (bars >= 2) gfx->fillRect(x + 1 * (barW + gap), baseY - h2, barW, h2, fg);
  if (bars >= 3) gfx->fillRect(x + 2 * (barW + gap), baseY - h3, barW, h3, fg);
}
