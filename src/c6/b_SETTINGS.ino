// NVS-backed settings. Namespace and key names match the M5 firmware so
// the same Web UI conventions carry over; new C6-specific keys are added
// below alongside the legacy ones.

void loadSettings()
{
  preferences.begin("vMixTally", false);

  if (preferences.getString("wifi_ssid").length() > 0) {
    WIFI_SSID = preferences.getString("wifi_ssid");
    WIFI_PASS = preferences.getString("wifi_pass");
  }

  if (preferences.getString("vmix_ip").length() > 0) {
    TALLY_NR = preferences.getUInt("tally", TALLY_NR);
    VMIX_IP  = preferences.getString("vmix_ip");
    if (TALLY_NR > 9) tnlen = 2;
  }

  if (preferences.isKey("conn_int"))  CONN_INT  = preferences.getUInt("conn_int");
  if (preferences.isKey("mode"))      MODE      = preferences.getUInt("mode");
  if (preferences.isKey("justLive"))  JUSTLIVE  = preferences.getUInt("justLive");

  if (preferences.isKey("ring_on"))   RING_ENABLE       = preferences.getUInt("ring_on");
  if (preferences.isKey("ring_br"))   RING_BRIGHTNESS   = preferences.getUInt("ring_br");
  if (preferences.isKey("ring_pre"))  RING_SHOW_PREVIEW = preferences.getUInt("ring_pre");
  if (preferences.isKey("ring_lo"))   RING_ONLY_LIVE    = preferences.getUInt("ring_lo");
  if (preferences.isKey("ring_n"))    RING_NUM          = preferences.getUInt("ring_n");

  if (preferences.isKey("prev_url"))  PREVIEW_URL = preferences.getString("prev_url");
  if (preferences.isKey("prev_hz"))   PREVIEW_HZ  = preferences.getUInt("prev_hz");

  if (preferences.isKey("tz"))        TZ_OFFSET   = (int)preferences.getInt("tz");
  if (preferences.isKey("sh_rec"))    SHOW_REC    = preferences.getUInt("sh_rec");
  if (preferences.isKey("sh_stm"))    SHOW_STREAM = preferences.getUInt("sh_stm");
  if (preferences.isKey("sh_clk"))    SHOW_CLOCK  = preferences.getUInt("sh_clk");

  if (preferences.getString("m_tally").length() > 0) {
    M_TALLY = preferences.getString("m_tally");
  }

  if (preferences.getUInt("bright")) {
    BRIGHTNESS = preferences.getUInt("bright");
    setBacklightPct(brightnessPctFromVar(BRIGHTNESS));
  }
  preferences.end();
}

void saveWiFiPreferences(String wifi_ssid, String wifi_pass)
{
  preferences.begin("vMixTally", false);
  if (wifi_ssid != "") {
    preferences.putString("wifi_ssid", wifi_ssid);
    preferences.putString("wifi_pass", wifi_pass);
  }
  preferences.end();
  WIFI_SSID = wifi_ssid;
  WIFI_PASS = wifi_pass;
}

void resetSettings() {
  preferences.begin("vMixTally", false);
  preferences.clear();
  preferences.end();

  resetScreen();
  gfx->println("Settings are reset");
  gfx->println();
  gfx->println("Please reboot device");
}

void increaseTally() {
  TALLY_NR++;
  preferences.begin("vMixTally", false);
  preferences.putUInt("tally", TALLY_NR);
  preferences.end();
}
void resetTally() {
  TALLY_NR = 1;
  preferences.begin("vMixTally", false);
  preferences.putUInt("tally", TALLY_NR);
  preferences.end();
}

void saveBrightness() {
  preferences.begin("vMixTally", false);
  preferences.putUInt("bright", BRIGHTNESS);
  preferences.end();
}

void printSettings() {
  Serial.printf("SSID:%s vMix:%s tally:%d preview:%s hz:%d\n",
    WIFI_SSID.c_str(), VMIX_IP.c_str(), TALLY_NR,
    PREVIEW_URL.c_str(), PREVIEW_HZ);
}
