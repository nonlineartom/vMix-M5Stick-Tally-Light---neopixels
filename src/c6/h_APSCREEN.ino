void showAPScreen() {
  resetScreen();
  screen = 4;
  gfx->fillScreen(BLACK);
  gfx->setTextColor(WHITE, BLACK);

  int cx = gfx->width() / 2;
  int y = 6;

  gfx->setTextSize(1);
  String title = "NO WIFI";
  gfx->setCursor(cx - textPxWidth(title, 1) / 2, y); gfx->println(title);
  y += 14;

  String sub = "Unable to connect to WiFi";
  gfx->setCursor(cx - textPxWidth(sub, 1) / 2, y); gfx->println(sub);
  y += 18;

  String ssidLbl = "SSID";
  gfx->setCursor(cx - textPxWidth(ssidLbl, 1) / 2, y); gfx->println(ssidLbl);
  y += 12;

  gfx->setTextSize(2);
  String mac = WiFi.macAddress();
  String ssidVal = String("BRONTIDE-Tally-") + mac.substring(mac.length() - 5);
  gfx->setCursor(cx - textPxWidth(ssidVal, 2) / 2, y); gfx->println(ssidVal);
  y += 22;

  String pwd = "PWD: 12345678";
  gfx->setCursor(cx - textPxWidth(pwd, 2) / 2, y); gfx->println(pwd);
  y += 22;

  gfx->setTextSize(1);
  String open = "Open in browser:";
  gfx->setCursor(cx - textPxWidth(open, 1) / 2, y); gfx->println(open);
  y += 12;

  gfx->setTextSize(2);
  String url = "192.168.4.1";
  gfx->setCursor(cx - textPxWidth(url, 2) / 2, y); gfx->println(url);
}
