void showTallyNum() {
  screen = 2;
  resetScreen();
  gfx->setTextSize(8);
  String s = String(TALLY_NR);
  int w = textPxWidth(s, 8);
  int x = (gfx->width() - w) / 2;
  int y = (gfx->height() - 8 * 8) / 2;
  gfx->setCursor(x, y);
  gfx->print(s);
}
