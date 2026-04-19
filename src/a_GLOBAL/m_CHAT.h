// Inter-stick "BORED" chat over UDP broadcast.
//
// Double-tap the M5 button on any stick → every other stick on the LAN
// running this firmware briefly displays BORED (or a randomised variant)
// over its tally screen, keeping the tally colour underneath. If a
// recipient double-taps back within 20 s, the original sender sees a
// ME TOO overlay.
//
// Constraints baked in:
//  * Never touches the NeoPixel ring (client-facing, must stay predictable).
//  * Never blocks longer than the standard loop() tick.
//  * Only paints on screen == 0 (tally). Other screens ignore chat events.
//  * A fresh tally state from vMix always wins against an active overlay.
#ifndef M_CHAT_H
#define M_CHAT_H

#include <WiFi.h>
#include <WiFiUdp.h>

// Declared in c_MAIN.ino + a_GLOBAL.ino — we read them, never write.
extern char screen;
extern char currentState;
extern int TALLY_NR;
extern void showTallyScreen();
extern uint16_t getBatteryBgColor();
extern uint16_t getBatteryTextColor();

// ---- tunables ----
#define CHAT_UDP_PORT            41234
#define CHAT_SENT_OVERLAY_MS     2000     // "message sent" dwell
#define CHAT_RECEIVED_OVERLAY_MS 20000    // BORED dwell + reply window
#define CHAT_METOO_OVERLAY_MS    3000     // "ME TOO" dwell
#define CHAT_REPLY_WINDOW_MS     20000    // awaitingReply deadline on sender

enum ChatOverlay {
  CHAT_OVERLAY_NONE,
  CHAT_OVERLAY_SENT,
  CHAT_OVERLAY_RECEIVED,
  CHAT_OVERLAY_METOO
};

static WiFiUDP chatUdp;
static bool chatReady = false;
static char chatMyMac[7] = "000000";                  // last 6 hex of our MAC
static ChatOverlay chatOverlay = CHAT_OVERLAY_NONE;
static bool chatOverlayPainted = false;
static unsigned long chatOverlayUntil = 0;
static unsigned long chatAwaitingReplyUntil = 0;      // we sent BORED; accept METOO until this
static char chatPeerMac[7] = "000000";                // address for our next METOO reply
static int  chatPeerTally = 0;                        // camera id shown in overlay
static char chatOverlayWord[10] = "BORED";            // random pick for this event
static int  chatBoredCount = 0;                       // fire-count badge

// Random pool of poke words — wire tag stays MSG, each stick shows whatever
// word the sender rolled so everyone sees the same cue.
static const char* const CHAT_WORDS[] = {
  "BORED", "YAWN", "HALP", "HELLO", "ZZZ", "POKE"
};
static const int CHAT_WORD_COUNT = sizeof(CHAT_WORDS) / sizeof(CHAT_WORDS[0]);

// ---- helpers ----
static inline void chatBuildMac() {
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  String tail = mac.substring(mac.length() - 6);
  tail.toUpperCase();
  tail.toCharArray(chatMyMac, sizeof(chatMyMac));
}

static inline void chatSetOverlay(ChatOverlay o, unsigned long durationMs) {
  chatOverlay = o;
  chatOverlayUntil = millis() + durationMs;
  chatOverlayPainted = false;
}

static inline void chatSendDatagram(const String& line) {
  if (!chatReady) return;
  chatUdp.beginPacket(IPAddress(255, 255, 255, 255), CHAT_UDP_PORT);
  chatUdp.write((const uint8_t*)line.c_str(), line.length());
  chatUdp.endPacket();
}

// ---- public API ----

static inline void chatBegin() {
  chatBuildMac();
}

static inline void chatOnWiFiReady() {
  if (chatReady) return;
  if (chatUdp.begin(CHAT_UDP_PORT)) {
    chatReady = true;
    Serial.print("CHAT UDP listening on ");
    Serial.print(WiFi.localIP());
    Serial.print(":");
    Serial.println(CHAT_UDP_PORT);
  }
}

static inline bool chatOverlayActive() {
  return chatOverlay != CHAT_OVERLAY_NONE;
}

// Called from d_VMIX.ino when a fresh tally state is drawn — lets the live
// show win against any chat overlay still on screen.
static inline void chatForceDismiss() {
  chatOverlay = CHAT_OVERLAY_NONE;
  chatOverlayUntil = 0;
  chatOverlayPainted = false;
}

static inline int chatBoredBadgeCount() {
  return chatBoredCount;
}

// Called when btnM5 double-tap fires.
//  * If a BORED is currently on our screen (within the 20 s reply window),
//    reply to its sender with ME TOO.
//  * Otherwise broadcast a fresh BORED.
static inline void chatOnM5DoubleTap() {
  if (!chatReady) return;

  if (chatOverlay == CHAT_OVERLAY_RECEIVED && millis() < chatOverlayUntil) {
    // Reply to whoever just poked us.
    String line = String("M5TALLY/1/METOO/")
                + chatMyMac + "/"
                + String(TALLY_NR) + "/"
                + chatPeerMac;
    chatSendDatagram(line);
    chatForceDismiss();
    return;
  }

  // Fresh BORED.
  const char* word = CHAT_WORDS[random(CHAT_WORD_COUNT)];
  strncpy(chatOverlayWord, word, sizeof(chatOverlayWord) - 1);
  chatOverlayWord[sizeof(chatOverlayWord) - 1] = '\0';

  String line = String("M5TALLY/1/MSG/")
              + chatMyMac + "/"
              + String(TALLY_NR) + "/"
              + word;
  chatSendDatagram(line);

  chatSetOverlay(CHAT_OVERLAY_SENT, CHAT_SENT_OVERLAY_MS);
  chatAwaitingReplyUntil = millis() + CHAT_REPLY_WINDOW_MS;
}

// Parse one incoming packet. Returns void; all effects go into module state.
static inline void chatHandlePacket(const char* buf, int len) {
  // Validate prefix.
  if (len < 10 || strncmp(buf, "M5TALLY/1/", 10) != 0) return;

  // Copy into a mutable buffer so strtok can work.
  char tmp[128];
  int n = len < (int)sizeof(tmp) - 1 ? len : (int)sizeof(tmp) - 1;
  memcpy(tmp, buf, n);
  tmp[n] = '\0';

  // Skip the prefix we already matched.
  char* p = tmp + 10;
  char* kind = strtok(p, "/");
  if (!kind) return;

  if (strcmp(kind, "MSG") == 0) {
    char* senderMac   = strtok(NULL, "/");
    char* senderTally = strtok(NULL, "/");
    char* word        = strtok(NULL, "/");
    if (!senderMac || !senderTally || !word) return;
    if (strcmp(senderMac, chatMyMac) == 0) return;     // self-echo

    strncpy(chatPeerMac, senderMac, sizeof(chatPeerMac) - 1);
    chatPeerMac[sizeof(chatPeerMac) - 1] = '\0';
    chatPeerTally = atoi(senderTally);

    // Sanitise word (alnum only, cap length).
    int wl = 0;
    for (; word[wl] && wl < (int)sizeof(chatOverlayWord) - 1; wl++) {
      char c = word[wl];
      if (!(isalnum((unsigned char)c))) { wl = 0; break; }
      chatOverlayWord[wl] = c;
    }
    if (wl == 0) strcpy(chatOverlayWord, "BORED");
    else chatOverlayWord[wl] = '\0';

    chatBoredCount++;
    chatSetOverlay(CHAT_OVERLAY_RECEIVED, CHAT_RECEIVED_OVERLAY_MS);
    return;
  }

  if (strcmp(kind, "METOO") == 0) {
    char* fromMac   = strtok(NULL, "/");
    char* fromTally = strtok(NULL, "/");
    char* toMac     = strtok(NULL, "/");
    if (!fromMac || !fromTally || !toMac) return;
    if (strcmp(toMac, chatMyMac) != 0) return;          // not addressed to us
    if (millis() > chatAwaitingReplyUntil) return;      // stale — we already moved on

    chatPeerTally = atoi(fromTally);
    chatSetOverlay(CHAT_OVERLAY_METOO, CHAT_METOO_OVERLAY_MS);
    chatAwaitingReplyUntil = 0;
    return;
  }
}

// Drain any pending UDP packets and expire overlays. Must be cheap — called
// every loop() iteration.
static inline void chatTick() {
  if (chatReady) {
    int sz;
    while ((sz = chatUdp.parsePacket()) > 0) {
      char buf[128];
      int n = chatUdp.read(buf, sizeof(buf) - 1);
      if (n > 0) {
        buf[n] = '\0';
        chatHandlePacket(buf, n);
      }
    }
  }

  // Expire overlay. If we had a RECEIVED overlay we auto-dismiss at the end
  // of its window. SENT and METOO follow the same pattern.
  if (chatOverlay != CHAT_OVERLAY_NONE && millis() >= chatOverlayUntil) {
    chatOverlay = CHAT_OVERLAY_NONE;
    chatOverlayUntil = 0;
    // Nudge the tally screen back into a clean state.
    if (screen == 0) showTallyScreen();
  }
}

// Draws the current overlay on the tally screen. Idempotent — only actually
// paints on the state transition; subsequent loop() calls are no-ops until
// the overlay is dismissed or replaced.
static inline void chatDrawOverlay() {
  if (chatOverlayPainted) return;
  chatOverlayPainted = true;
  uint16_t bg = getBatteryBgColor();
  uint16_t fg = getBatteryTextColor();

  M5.Lcd.fillScreen(bg);
  M5.Lcd.setTextColor(fg, bg);

  const char* headline = "";
  bool showFrom = false;

  switch (chatOverlay) {
    case CHAT_OVERLAY_SENT:     headline = "SENT";            break;
    case CHAT_OVERLAY_RECEIVED: headline = chatOverlayWord;   showFrom = true; break;
    case CHAT_OVERLAY_METOO:    headline = "ME TOO";          showFrom = true; break;
    default: return;
  }

  M5.Lcd.setTextSize(3);
  int w = M5.Lcd.textWidth(headline);
  int x = (M5.Lcd.width()  - w) / 2;
  int y = (M5.Lcd.height() - 24) / 2 - 12;
  M5.Lcd.setCursor(x, y);
  M5.Lcd.print(headline);

  if (showFrom) {
    String sub = "from cam " + String(chatPeerTally);
    M5.Lcd.setTextSize(1);
    int sw = M5.Lcd.textWidth(sub);
    int sx = (M5.Lcd.width() - sw) / 2;
    int sy = y + 28;
    M5.Lcd.setCursor(sx, sy);
    M5.Lcd.print(sub);
  }
}

// Small tally-screen badge: "b:N" bottom-left, only when the count is > 0.
// Called from showTallyScreen() after the main render so it layers on top.
static inline void chatDrawBoredBadge() {
  if (chatBoredCount <= 0) return;
  uint16_t bg = getBatteryBgColor();
  uint16_t fg = getBatteryTextColor();
  String badge = "b:" + String(chatBoredCount);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(fg, bg);
  M5.Lcd.setCursor(2, M5.Lcd.height() - 10);
  M5.Lcd.print(badge);
}

#endif
