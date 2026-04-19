// WEBSERVER STUFF
String HEADER = "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no, shrink-to-fit=no'><title>vMix M5Stick-C Tally</title><style>@import url(https://fonts.googleapis.com/css2?family=Open+Sans&display=swap);.wrapper,input[type=text],input[type=number],input[type=submit],select{width:100%;box-sizing:border-box}body,html{background:#2b2b2b;color:#eee;padding:0;margin:0;font-family:'Open Sans',verdana,sans-serif}.wrapper{padding:10px}.wrapper h1{text-align:center}input[type=text],input[type=number],select{margin-bottom:10px}input,select{background-color:#6d6d6d;color:#f0f0f0;border:1px solid #000;font-size:18px;height:35px;padding:0 5px}input[type=submit]{height:50px;margin:0 auto}@media screen and (min-width:600px){.wrapper{width:600px;margin:0 auto}}</style></head><body><div class='wrapper'>";

String FOOTER = "</div></body></html>";

void handle_root()
{
    String tally = (String)TALLY_NR;
    String justLive = (String)JUSTLIVE;
    String bright = (String)BRIGHTNESS;
    String _mode = (String)MODE;
    String ringOn = (String)RING_ENABLE;
    String ringBr = (String)RING_BRIGHTNESS;
    String ringPre = (String)RING_SHOW_PREVIEW;
    String ringLo = (String)RING_ONLY_LIVE;
    String HTML = HEADER;

    HTML += "<div class=wrapper data-theme=light><h1>vMix M5Stack Tally Settings</h1><form action=/save id=frmData method=post onsubmit=return!1><div>SSID:<br><select id=ssid><option disabled selected>Scanning wifi...</option></select></div><div class=ssidCustomDiv style=display:none>Hidden SSID Name:<br><input id=ssidCustom type=text value='" + (String)WIFI_SSID + "'name=ssidCustom></div><div>Password:<br><input id=pwd type=text value='" + (String)WIFI_PASS + "'name=pwd></div><div>vMix IP Address:<br><input id=vmixip type=text value='" + (String)VMIX_IP + "'name=vmixip></div><div>Main Tally Number:<br><input id=tally_num type=number value='" + tally + "'name=tally_num max=1000 min=1></div><div>Multi Input (comma separated):<br><input id=m_tally type=text value='" + (String)M_TALLY + "'name=m_tally></div><div>Reconnect interval (in seconds, 0 means no reconnection interval):<br><input id=conn_int type=number value='" + CONN_INT + "'name=conn_int></div><div>Brightness:<br><select id=drpBright name=bright><option value=7>10%</option><option value=8>20%</option><option value=9>40%</option><option value=10>60%</option><option value=11>80%</option><option value=12>100%</option></select></div><div>Just Live:<br><select id=drpJustLive name=justLive><option value=0>False</option><option value=1>True</option></select></div><div>Mode:<br><select id=drpMode name=mode><option value=0>Text (SAFE, PRE, LIVE)</option><option value=1>Tally Number</option></select></div><h2>NeoPixel Ring</h2><div>Ring Enabled:<br><select id=drpRingOn name=ring_on><option value=0>False</option><option value=1>True</option></select></div><div>Ring Brightness (0-100):<br><input id=ring_br type=number value='" + ringBr + "'name=ring_br min=0 max=100></div><div>Show Preview (green):<br><select id=drpRingPre name=ring_pre><option value=0>False</option><option value=1>True</option></select></div><div>Only Light on LIVE (red):<br><select id=drpRingLo name=ring_lo><option value=0>False</option><option value=1>True</option></select></div><input id=btnSave type=submit value=SAVE class='btn btn-primary'></form><h2>Reconnect to vMix</h2><form action=/reconnect id=frmReconnect method=post onsubmit=return!1><input id=btnReconnect type=submit value=RECONNECT></form></div><script>const btnSave = document.querySelector('#btnSave'); const btnReconnect = document.querySelector('#btnReconnect'); const drpBright = document.querySelector('#drpBright'); const ssidSelect = document.querySelector('#ssid'); const drpJustLive = document.querySelector('#drpJustLive'); const drpMode = document.querySelector('#drpMode'); const drpRingOn = document.querySelector('#drpRingOn'); const drpRingPre = document.querySelector('#drpRingPre'); const drpRingLo = document.querySelector('#drpRingLo'); drpBright.value = '" + bright + "'; drpJustLive.value = '" + justLive + "'; drpMode.value = '" + _mode + "'; drpRingOn.value = '" + ringOn + "'; drpRingPre.value = '" + ringPre + "'; drpRingLo.value = '" + ringLo + "'; btnSave.addEventListener('click', async function(e){ e.preventDefault(); let ssid = document.querySelector('#ssid').value; if(ssid === '__hidden__'){ ssid = document.querySelector('#ssidCustom').value; } const pwd = document.querySelector('#pwd').value; const vmixip = document.querySelector('#vmixip').value; const m_tally = document.querySelector('#m_tally').value; const frmData = document.querySelector('#frmData'); const tally_num = document.querySelector('#tally_num').value; const bright = drpBright.value; const mode = drpMode.value; const justLive = drpJustLive.value; const ring_on = drpRingOn.value; const ring_br = document.querySelector('#ring_br').value; const ring_pre = drpRingPre.value; const ring_lo = drpRingLo.value; const conn_int = parseInt(document.querySelector('#conn_int').value, 10) || 0; let formData = new FormData(); formData.append('ssid', ssid.trim()); formData.append('pwd',pwd.trim()); formData.append('vmixip', vmixip.trim()); formData.append('m_tally', m_tally.trim().replace(/[^0-9,]+/g, '')); formData.append('tally_num', tally_num); formData.append('conn_int', conn_int); formData.append('bright', bright); formData.append('mode', mode); formData.append('justLive', justLive); formData.append('ring_on', ring_on); formData.append('ring_br', ring_br); formData.append('ring_pre', ring_pre); formData.append('ring_lo', ring_lo); btnSave.setAttribute('disabled', ''); const res = await fetch('/save', { method: 'POST', cache: 'no-cache', referrerPolicy: 'no-referrer', body: formData }); if(res.status === 200){ btnSave.value = 'SETTINGS SAVED!'; await setTimeout(()=>{btnSave.value = 'SAVE';}, 3000); } btnSave.removeAttribute('disabled'); }); btnReconnect.addEventListener('click', function(e){ e.preventDefault(); fetch('/reconnect'); }); ssidSelect.addEventListener('change', e => { const val = e.target.value; const ssidcd = document.querySelector('.ssidCustomDiv'); if(val === '__hidden__'){ ssidcd.style.display = 'block'; } else { ssidcd.style.display = 'none'; } }); document.addEventListener('DOMContentLoaded', async function(){ const res = await fetch('/scanNetwork'); res.text().then(text=>{ let networks = [text]; let str = ''; if(text.indexOf('|||') !== -1){ networks = text.split('|||'); } let sel = document.getElementById('ssid'); sel.innerHTML = ''; let existingNetwork = ''; networks.forEach(network => { let opt = document.createElement('option'); opt.appendChild( document.createTextNode(network) ); opt.value = network; if('" + (String)WIFI_SSID + "' === network){ existingNetwork = network; } sel.appendChild(opt); }); let opt = document.createElement('option'); opt.appendChild( document.createTextNode('Hidden network') ); opt.value = '__hidden__'; sel.appendChild(opt); if(existingNetwork !== ''){ sel.value = existingNetwork; } }); });</script>";
    HTML += FOOTER;

    server.send(200, "text/html", HTML);
}

void handle_save()
{
    String message = "";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); i++)
    {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    message += " direct:" + server.arg("tally_num");
    server.send(200, "text/plain", message);
    Serial.println(message);

    String tally = server.arg("tally_num");
    String bright = server.arg("bright");
    String justLive = server.arg("justLive");
    String _mode = server.arg("mode");
    String conn_int = server.arg("conn_int");
    String ring_on = server.arg("ring_on");
    String ring_br = server.arg("ring_br");
    String ring_pre = server.arg("ring_pre");
    String ring_lo = server.arg("ring_lo");

    Serial.print("BRIGHTNESS: ");
    Serial.println(bright);

    // save value in preferences
    preferences.begin("vMixTally", false);
    if (tally != "")
    {
        TALLY_NR = std::atoi(tally.c_str());
        preferences.putUInt("tally", TALLY_NR);
        Serial.println("PUT TALLY NR");
    }
    if(bright != ""){
      BRIGHTNESS = std::atoi(bright.c_str());
      preferences.putUInt("bright", BRIGHTNESS);
      Serial.println("PUT BRIGHT");
    }
    if(justLive != ""){
      JUSTLIVE = std::atoi(justLive.c_str());
      preferences.putUInt("justLive", JUSTLIVE);
      Serial.println("PUT JUSTLIVE");
    }
    if(_mode != ""){
      MODE = std::atoi(_mode.c_str());
      preferences.putUInt("mode", MODE);
      Serial.println("PUT MODE");
    }

    if(ring_on != ""){
      RING_ENABLE = std::atoi(ring_on.c_str());
      preferences.putUInt("ring_on", RING_ENABLE);
    }
    if(ring_br != ""){
      RING_BRIGHTNESS = constrain(std::atoi(ring_br.c_str()), 0, 100);
      preferences.putUInt("ring_br", RING_BRIGHTNESS);
    }
    if(ring_pre != ""){
      RING_SHOW_PREVIEW = std::atoi(ring_pre.c_str());
      preferences.putUInt("ring_pre", RING_SHOW_PREVIEW);
    }
    if(ring_lo != ""){
      RING_ONLY_LIVE = std::atoi(ring_lo.c_str());
      preferences.putUInt("ring_lo", RING_ONLY_LIVE);
    }


    if (server.arg("ssid") != "")
    {
        WIFI_SSID = server.arg("ssid");
        WIFI_PASS = server.arg("pwd");
        preferences.putString("wifi_ssid", &(WIFI_SSID[0]));
        preferences.putString("wifi_pass", &(WIFI_PASS[0]));
        Serial.println("PUT WIFI_SSID & WIFI PASS");
    }

    M_TALLY = server.arg("m_tally");
    preferences.putString("m_tally", &(M_TALLY[0]));

    if (conn_int != "")
    {
        CONN_INT = std::atoi(conn_int.c_str());
        preferences.putUInt("conn_int", CONN_INT);
        Serial.println("PUT CONN INT");
    }
    
    if (server.arg("vmixip") != "")
    {
        VMIX_IP = server.arg("vmixip");
        preferences.putString("vmix_ip", &(VMIX_IP[0]));
        Serial.println("PUT VMIX IP");
    }
    preferences.end();
    
    cls();

    //Reboot stick
    ESP.restart(); //Thanks to Babbit on vMix forums!
}

void handleReconnect(){
  connectTovMix(false);
  server.send(200, "text/plain", "success");
}

void handleScanNetwork(){
  int numSsid = WiFi.scanNetworks();
  if(numSsid == -1){
    
  } else {
    String retStr = "";
    for (int thisNet = 0; thisNet<numSsid; thisNet++) {
      if(thisNet == 0){
        retStr += WiFi.SSID(thisNet);
      } else {
        retStr += "|||" + WiFi.SSID(thisNet);
      }
    }
    server.send(200, "text/plain", retStr);
  }
}

void startServer()
{
    server.on("/", handle_root);
    server.on("/save", handle_save);
    server.on("/reconnect", handleReconnect);
    server.on("/scanNetwork", handleScanNetwork);
    server.begin();
    Serial.println("HTTP server started");
}
