void showBrightnessScreen() {
  screen = 3;
  resetScreen();
  gfx->fillScreen(WHITE);
  gfx->setTextColor(BLACK, WHITE);

  gfx->setTextSize(2);
  String title1 = "ADJUST";
  String title2 = "BRIGHTNESS";
  int cx = gfx->width() / 2;

  gfx->setCursor(cx - textPxWidth(title1, 2) / 2, 10); gfx->println(title1);
  gfx->setCursor(cx - textPxWidth(title2, 2) / 2, 32); gfx->println(title2);

  int arrowX = cx, arrowTop = 60;
  gfx->drawLine(arrowX, arrowTop, arrowX, arrowTop + 20, BLACK);
  gfx->fillTriangle(arrowX, arrowTop + 30, arrowX - 10, arrowTop + 20, arrowX + 10, arrowTop + 20, BLACK);

  int pct = brightnessPctFromVar(BRIGHTNESS);
  String s = String(pct) + "%";
  gfx->setTextSize(3);
  int pw = textPxWidth(s, 3);
  gfx->setCursor(cx - pw / 2, arrowTop + 40);
  gfx->print(s);
}

void updateBrightnessVar() {
  if (BRIGHTNESS == 12)      BRIGHTNESS = 7;
  else if (BRIGHTNESS == 7)  BRIGHTNESS = 8;
  else if (BRIGHTNESS == 8)  BRIGHTNESS = 9;
  else if (BRIGHTNESS == 9)  BRIGHTNESS = 10;
  else if (BRIGHTNESS == 10) BRIGHTNESS = 11;
  else if (BRIGHTNESS == 11) BRIGHTNESS = 12;
  else                       BRIGHTNESS = 12;
  updateBrightness();
  showBrightnessScreen();
}

void updateBrightness() {
  setBacklightPct(brightnessPctFromVar(BRIGHTNESS));
  saveBrightness();
}
