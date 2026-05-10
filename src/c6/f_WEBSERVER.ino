// Tiny embedded HTTP settings UI. Same shape as the M5 build, with the
// new C6-specific fields (preview proxy URL, preview rate, ring count,
// status-strip toggles, NTP offset) appended at the bottom.

static const char *HEADER =
  "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'>"
  "<meta name='viewport' content='width=device-width,initial-scale=1'>"
  "<title>BRONTIDE Tally</title>"
  "<style>body{background:#222;color:#eee;font-family:sans-serif;margin:0;padding:10px}"
  ".w{max-width:600px;margin:0 auto}h1,h2{text-align:center}"
  "input,select{width:100%;box-sizing:border-box;background:#444;color:#eee;"
  "border:1px solid #000;font-size:16px;height:34px;padding:4px;margin-bottom:8px}"
  "input[type=submit]{height:46px;background:#0a7;border:0}"
  "label{display:block;margin-top:6px;font-size:14px;color:#bbb}</style></head>"
  "<body><div class='w'>";
static const char *FOOTER = "</div></body></html>";

static String selOpt(const char *v, const char *label, int cur) {
  String s = "<option value='"; s += v; s += "'";
  if (atoi(v) == cur) s += " selected";
  s += ">"; s += label; s += "</option>";
  return s;
}

void handle_root() {
  String h = HEADER;
  h += "<h1>BRONTIDE Tally Settings</h1>";
  h += "<form action='/save' method='post'>";

  h += "<h2>Network</h2>";
  h += "<label>SSID</label><select name='ssid' id='ssid'><option>scanning…</option></select>";
  h += "<label>Hidden SSID (optional)</label><input name='ssidCustom' value='" + WIFI_SSID + "'>";
  h += "<label>Password</label><input name='pwd' value='" + WIFI_PASS + "'>";

  h += "<h2>vMix</h2>";
  h += "<label>vMix IP</label><input name='vmixip' value='" + VMIX_IP + "'>";
  h += "<label>Tally number</label><input type='number' min='1' max='1000' name='tally_num' value='" + String(TALLY_NR) + "'>";
  h += "<label>Multi tally (comma list)</label><input name='m_tally' value='" + M_TALLY + "'>";
  h += "<label>Reconnect interval (s, 0=off)</label><input type='number' name='conn_int' value='" + String(CONN_INT) + "'>";

  h += "<h2>Display</h2>";
  h += "<label>Brightness</label><select name='bright'>";
  h += selOpt("7","10%",BRIGHTNESS) + selOpt("8","20%",BRIGHTNESS) + selOpt("9","40%",BRIGHTNESS);
  h += selOpt("10","60%",BRIGHTNESS) + selOpt("11","80%",BRIGHTNESS) + selOpt("12","100%",BRIGHTNESS);
  h += "</select>";
  h += "<label>Mode</label><select name='mode'>";
  h += selOpt("0","Text (SAFE/PRE/LIVE)",MODE) + selOpt("1","Tally number",MODE);
  h += "</select>";
  h += "<label>Just Live</label><select name='justLive'>";
  h += selOpt("0","No",JUSTLIVE) + selOpt("1","Yes",JUSTLIVE) + "</select>";

  h += "<h2>Status strip</h2>";
  h += "<label>Show REC</label><select name='sh_rec'>";
  h += selOpt("0","No",SHOW_REC) + selOpt("1","Yes",SHOW_REC) + "</select>";
  h += "<label>Show STREAM</label><select name='sh_stm'>";
  h += selOpt("0","No",SHOW_STREAM) + selOpt("1","Yes",SHOW_STREAM) + "</select>";
  h += "<label>Show wall clock</label><select name='sh_clk'>";
  h += selOpt("0","No",SHOW_CLOCK) + selOpt("1","Yes",SHOW_CLOCK) + "</select>";
  h += "<label>NTP offset (minutes from UTC)</label><input type='number' name='tz' value='" + String(TZ_OFFSET) + "'>";

  h += "<h2>Live preview (optional)</h2>";
  h += "<label>Proxy URL (empty disables)<br><span style=color:#888>e.g. http://10.0.0.5:8088/preview/1?w=320&amp;h=110</span></label>";
  h += "<input name='prev_url' value='" + PREVIEW_URL + "'>";
  h += "<label>Poll rate (Hz, 1-15)</label><input type='number' min='1' max='15' name='prev_hz' value='" + String(PREVIEW_HZ) + "'>";

  h += "<h2>NeoPixel ring</h2>";
  h += "<label>Enabled</label><select name='ring_on'>";
  h += selOpt("0","No",RING_ENABLE) + selOpt("1","Yes",RING_ENABLE) + "</select>";
  h += "<label>LED count (1=onboard only)</label><input type='number' min='1' max='64' name='ring_n' value='" + String(RING_NUM) + "'>";
  h += "<label>Brightness (0-100)</label><input type='number' min='0' max='100' name='ring_br' value='" + String(RING_BRIGHTNESS) + "'>";
  h += "<label>Show preview (green)</label><select name='ring_pre'>";
  h += selOpt("0","No",RING_SHOW_PREVIEW) + selOpt("1","Yes",RING_SHOW_PREVIEW) + "</select>";
  h += "<label>Only on LIVE</label><select name='ring_lo'>";
  h += selOpt("0","No",RING_ONLY_LIVE) + selOpt("1","Yes",RING_ONLY_LIVE) + "</select>";

  h += "<input type='submit' value='SAVE &amp; REBOOT'>";
  h += "</form><form action='/reconnect' method='post'><input type='submit' value='Reconnect to vMix'></form>";
  h += "<script>fetch('/scanNetwork').then(r=>r.text()).then(t=>{"
       "let s=document.getElementById('ssid');s.innerHTML='';"
       "t.split('|||').forEach(n=>{let o=document.createElement('option');"
       "o.textContent=n;o.value=n;if(n==='" + WIFI_SSID + "')o.selected=true;s.appendChild(o)});"
       "let o=document.createElement('option');o.value='__hidden__';o.textContent='Hidden network';s.appendChild(o);"
       "});</script>";
  h += FOOTER;
  server.send(200, "text/html", h);
}

void handle_save() {
  preferences.begin("vMixTally", false);

  String ssid = server.arg("ssid");
  if (ssid == "__hidden__") ssid = server.arg("ssidCustom");
  if (ssid != "") {
    WIFI_SSID = ssid; WIFI_PASS = server.arg("pwd");
    preferences.putString("wifi_ssid", WIFI_SSID);
    preferences.putString("wifi_pass", WIFI_PASS);
  }

  if (server.arg("vmixip") != "")     { VMIX_IP = server.arg("vmixip"); preferences.putString("vmix_ip", VMIX_IP); }
  if (server.arg("tally_num") != "")  { TALLY_NR = server.arg("tally_num").toInt(); preferences.putUInt("tally", TALLY_NR); }
  M_TALLY = server.arg("m_tally");      preferences.putString("m_tally", M_TALLY);
  if (server.arg("conn_int") != "")   { CONN_INT  = server.arg("conn_int").toInt();  preferences.putUInt("conn_int", CONN_INT); }
  if (server.arg("bright") != "")     { BRIGHTNESS = server.arg("bright").toInt();   preferences.putUInt("bright", BRIGHTNESS); }
  if (server.arg("mode") != "")       { MODE      = server.arg("mode").toInt();      preferences.putUInt("mode", MODE); }
  if (server.arg("justLive") != "")   { JUSTLIVE  = server.arg("justLive").toInt();  preferences.putUInt("justLive", JUSTLIVE); }

  if (server.arg("ring_on") != "")    { RING_ENABLE       = server.arg("ring_on").toInt();  preferences.putUInt("ring_on", RING_ENABLE); }
  if (server.arg("ring_br") != "")    { RING_BRIGHTNESS   = constrain(server.arg("ring_br").toInt(), 0, 100); preferences.putUInt("ring_br", RING_BRIGHTNESS); }
  if (server.arg("ring_pre") != "")   { RING_SHOW_PREVIEW = server.arg("ring_pre").toInt(); preferences.putUInt("ring_pre", RING_SHOW_PREVIEW); }
  if (server.arg("ring_lo") != "")    { RING_ONLY_LIVE    = server.arg("ring_lo").toInt();  preferences.putUInt("ring_lo", RING_ONLY_LIVE); }
  if (server.arg("ring_n") != "")     { RING_NUM          = constrain(server.arg("ring_n").toInt(), 1, 64); preferences.putUInt("ring_n", RING_NUM); }

  PREVIEW_URL = server.arg("prev_url"); preferences.putString("prev_url", PREVIEW_URL);
  if (server.arg("prev_hz") != "")    { PREVIEW_HZ = constrain(server.arg("prev_hz").toInt(), 1, 15); preferences.putUInt("prev_hz", PREVIEW_HZ); }

  if (server.arg("tz") != "")         { TZ_OFFSET   = server.arg("tz").toInt();   preferences.putInt("tz", TZ_OFFSET); }
  if (server.arg("sh_rec") != "")     { SHOW_REC    = server.arg("sh_rec").toInt(); preferences.putUInt("sh_rec", SHOW_REC); }
  if (server.arg("sh_stm") != "")     { SHOW_STREAM = server.arg("sh_stm").toInt(); preferences.putUInt("sh_stm", SHOW_STREAM); }
  if (server.arg("sh_clk") != "")     { SHOW_CLOCK  = server.arg("sh_clk").toInt(); preferences.putUInt("sh_clk", SHOW_CLOCK); }

  preferences.end();
  server.send(200, "text/plain", "saved; rebooting");
  delay(200);
  ESP.restart();
}

void handleReconnect() {
  connectTovMix(false);
  server.send(200, "text/plain", "ok");
}

void handleScanNetwork() {
  int n = WiFi.scanNetworks();
  String r;
  for (int i = 0; i < n; i++) {
    if (i) r += "|||";
    r += WiFi.SSID(i);
  }
  server.send(200, "text/plain", r);
}

void startServer() {
  server.on("/",            handle_root);
  server.on("/save",        HTTP_POST, handle_save);
  server.on("/reconnect",   handleReconnect);
  server.on("/scanNetwork", handleScanNetwork);
  server.begin();
  Serial.println("HTTP server started");
}
