// vMix TCP protocol + tally render + status banner.
//
// Protocol is unchanged from the M5 firmware: connect to TCP/8099,
// SUBSCRIBE TALLY + ACTS, parse TALLY/ACTS lines. Rendering is sized for
// the 320x172 IPS panel and the optional preview-thumbnail layout.

#include <time.h>

bool stm = 0, rec = 0;
unsigned long recStart = 0, stmStart = 0;
static unsigned long _lastStatusDraw = 0;

boolean connectTovMix(bool recursive)
{
  resetScreen();
  Serial.println("Connecting to vMix...");
  gfx->println("Connecting to vMix...");

  if (client.connect(VMIX_IP.c_str(), VMIX_PORT)) {
    connectedTovMix = true;
    gfx->println("Connected to vMix!");
    ringSetStatus(RING_STATUS_VMIX_CONNECTED);
    client.println("SUBSCRIBE TALLY");
    client.println("SUBSCRIBE ACTS");
    showTallyScreen();
    return true;
  }
  if (recursive) return false;

  cls();
  gfx->println("Could not connect to vMix");
  gfx->println("Retrying: 0/3");
  for (uint8_t i = 0; i < 3; i++) {
    if (!retryConnectionvMix(i)) return true;
  }
  cls();
  connectedTovMix = false;
  noConnectionTovMix();
  return false;
}

void singleReconnect() {
  resetScreen();
  gfx->println("Connecting to vMix...");
  if (client.connect(VMIX_IP.c_str(), VMIX_PORT)) {
    lastConnCheck = millis();
    connectedTovMix = true;
    gfx->println("Connected to vMix!");
    client.println("SUBSCRIBE TALLY");
    client.println("SUBSCRIBE ACTS");
    showTallyScreen();
  } else {
    lastConnCheck = millis();
  }
}

boolean retryConnectionvMix(int tryCount) {
  cls();
  gfx->setTextSize(1);
  gfx->println("Couldn't connect to vMix");
  gfx->print("Retrying: "); gfx->print(tryCount); gfx->print("/3");
  delay(2000);
  return connectTovMix(true) ? false : true;
}

// Layout constants — top tally band height. When PREVIEW_URL is set we
// shrink the band so the JPEG fits below it.
static int tallyBandH() {
  return (PREVIEW_URL.length() > 0) ? 60 : 150;
}

static void drawTallyText(uint16_t bg, uint16_t fg, const char *txt) {
  gfx->fillRect(0, 0, gfx->width(), tallyBandH(), bg);
  gfx->setTextColor(fg, bg);

  if (MODE == 1) {
    String s = String(TALLY_NR);
    int sz = (PREVIEW_URL.length() > 0) ? 5 : 8;
    gfx->setTextSize(sz);
    int w = textPxWidth(s, sz);
    int h = 8 * sz;
    gfx->setCursor((gfx->width() - w) / 2, (tallyBandH() - h) / 2);
    gfx->print(s);
  } else {
    int sz = (PREVIEW_URL.length() > 0) ? 5 : 8;
    gfx->setTextSize(sz);
    int w = textPxWidth(txt, sz);
    int h = 8 * sz;
    gfx->setCursor((gfx->width() - w) / 2, (tallyBandH() - h) / 2);
    gfx->print(txt);
  }
}

void setTallyProgram() {
  drawTallyText(RED, WHITE, MODE == 0 ? "LIVE" : "");
  ringOnTally('1');
}
void setTallyPreview() {
  drawTallyText(GREEN, BLACK, (!JUSTLIVE && MODE == 0) ? "PRE" : "");
  ringOnTally('2');
}
void setTallyOff() {
  drawTallyText(BLACK, WHITE, (!JUSTLIVE && MODE == 0) ? "SAFE" : "");
  ringOnTally('0');
}

void handleData(String data) {
  bool changed = false;
  if (data.indexOf("TALLY") == 0) {
    char newState = data.charAt(TALLY_NR + 8);
    if (currentState != newState || screen == 1) {
      currentState = newState;
      changed = true;
      if (M_TALLY == "") showTallyScreen();
    }
    if (M_TALLY != "" && currentState != '1') {
      int len = M_TALLY.length() + 1;
      char buf[len]; M_TALLY.toCharArray(buf, len);
      char *pch = strtok(buf, ",");
      while (pch != NULL) {
        String c(pch);
        char ns = data.charAt(c.toInt() + 8);
        if ((currentState == '0' && (ns == '1' || ns == '2')) ||
            (currentState == '2' && ns == '1')) {
          currentState = ns;
          changed = true;
        }
        pch = strtok(NULL, ",");
      }
    }
    if (changed && M_TALLY != "") showTallyScreen();
  } else {
    if (data.startsWith("ACTS OK Recording 1")) { if (!rec) recStart = millis(); rec = 1; }
    else if (data.startsWith("ACTS OK Recording 0")) { rec = 0; }
    else if (data.startsWith("ACTS OK Streaming 1")) { if (!stm) stmStart = millis(); stm = 1; }
    else if (data.startsWith("ACTS OK Streaming 0")) { stm = 0; }
    else { Serial.print("vMix: "); Serial.println(data); }
    showStatus();
  }
}

void showTallyScreen() {
  cls();
  screen = 0;

  if (!JUSTLIVE) {
    switch (currentState) {
      case '0': setTallyOff();     break;
      case '1': setTallyProgram(); break;
      case '2': setTallyPreview(); break;
      default:  setTallyOff();
    }
  } else {
    if (currentState == '1') setTallyProgram();
    else                     setTallyOff();
  }

  // Battery + WiFi icons over the top band.
  renderBatteryLevel();
  drawWiFiIcon(gfx->width() - 30, 2);

  // Bottom status strip, regardless of preview presence.
  showStatus();
}

// Format millis duration as "mm:ss" or "h:mm:ss".
static String fmtDur(unsigned long start) {
  unsigned long s = (millis() - start) / 1000;
  unsigned long h = s / 3600;
  unsigned long m = (s / 60) % 60;
  unsigned long ss = s % 60;
  char buf[16];
  if (h > 0) snprintf(buf, sizeof(buf), "%lu:%02lu:%02lu", h, m, ss);
  else       snprintf(buf, sizeof(buf), "%02lu:%02lu", m, ss);
  return String(buf);
}

void showStatus() {
  if (millis() - _lastStatusDraw < 500) return;   // throttle
  _lastStatusDraw = millis();

  // Bottom 22 px strip — plain black, regardless of tally state, so the
  // text is always legible.
  int sh = 22;
  int sy = gfx->height() - sh;
  gfx->fillRect(0, sy, gfx->width(), sh, BLACK);
  gfx->setTextSize(2);
  gfx->setTextColor(WHITE, BLACK);

  int x = 4;
  if (SHOW_REC) {
    gfx->setTextColor(rec ? RED : 0x4208, BLACK);
    gfx->setCursor(x, sy + 4);
    gfx->print("\xE2\x97\x8F");  // not in built-in font; fall back to "R"
    // The built-in font is ASCII-only, so just use a letter.
    gfx->setCursor(x, sy + 4);
    gfx->print("R ");
    if (rec) gfx->print(fmtDur(recStart));
    else     gfx->print("--:--");
    x += 90;
  }
  if (SHOW_STREAM) {
    gfx->setTextColor(stm ? 0x07FF : 0x4208, BLACK);
    gfx->setCursor(x, sy + 4);
    gfx->print("S ");
    if (stm) gfx->print(fmtDur(stmStart));
    else     gfx->print("--:--");
    x += 90;
  }
  if (SHOW_CLOCK) {
    time_t now = time(nullptr);
    if (now > 1700000000) {       // sanity: NTP has happened
      struct tm tmv;
      localtime_r(&now, &tmv);
      char buf[12];
      snprintf(buf, sizeof(buf), "%02d:%02d:%02d", tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
      gfx->setTextColor(WHITE, BLACK);
      int w = textPxWidth(buf, 2);
      gfx->setCursor(gfx->width() - w - 4, sy + 4);
      gfx->print(buf);
    }
  }
}

void noConnectionTovMix() {
  resetScreen();
  gfx->println("Couldn't connect to vMix");
  gfx->println();
  gfx->println("vMix is closed");
  gfx->println("or check settings");
  gfx->println();
  gfx->print("http://");
  gfx->println(WiFi.localIP());
  gfx->println();
  gfx->println("Long-press BOOT to reconnect");
}
