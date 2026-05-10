void showNetworkScreen() {
  resetScreen();
  screen = 1;
  gfx->setTextSize(2);
  gfx->setCursor(0, 0);
  gfx->println("SSID:");
  gfx->println(WIFI_SSID);
  gfx->println();
  gfx->println("IP:");
  gfx->println(WiFi.localIP());
  gfx->println();
  gfx->print("RSSI: "); gfx->print(rssi); gfx->print(" ("); gfx->print(rssiLabel); gfx->println(")");
}
