// WiFi station + AP fallback.
//
// Behaves identically to the M5 firmware: try station for 10 s, fall back
// to a fixed-password AP and run the settings web UI on 192.168.4.1.
// On successful station connect we also kick NTP so the wall clock and
// the rec/stream timers can render correctly.

#include <time.h>

void startWiFi() {
  Serial.println("STARTING WIFI");
  ringSetStatus(RING_STATUS_WIFI_CONNECTING);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID.c_str(), WIFI_PASS.c_str());

  gfx->setTextSize(2);
  gfx->setCursor(8, gfx->height() - 32);
  gfx->println("Waiting for WiFi...");

  unsigned long startAttemptTime = millis();
  const unsigned long wifiTimeout = 10000;
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < wifiTimeout) {
    gfx->print(".");
    for (int i = 0; i < 10 && WiFi.status() != WL_CONNECTED; i++) {
      ringTick();
      delay(100);
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi failed; AP mode");
    startLocalWiFi();
  } else {
    cls();
    gfx->println("WiFi connected");
    gfx->println("IP address: ");
    gfx->print(WiFi.localIP());

    // NTP — 3600s offset is applied via TZ_OFFSET (minutes).
    long off = (long)TZ_OFFSET * 60;
    configTime(off, 0, "pool.ntp.org", "time.google.com");

    startServer();
    connectTovMix(false);
  }
}

void startLocalWiFi() {
  char apSsid[40];
  String mac = WiFi.macAddress();
  snprintf(apSsid, sizeof(apSsid), "BRONTIDE-Tally-%s",
           mac.substring(mac.length() - 5).c_str());
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSsid, "12345678");
  apEnabled = true;
  showAPScreen();
  delay(100);
  startServer();
}
