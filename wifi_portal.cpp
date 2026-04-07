#include <Arduino.h>
#include <WiFi.h>
#include <Update.h>
#include "wifi_portal.h"
#include "config_manager.h"

AsyncWebServer server(80);
DNSServer dnsServer;

bool configModeActive = false;
const char* ssid = "HID-Panel-Cfg";
const char* pass = "12345678";
IPAddress apIP(192, 168, 4, 1);

String getHead() { 
  return F("<html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'><style>body{font-family:sans-serif; background:#f0f2f5; margin:0; padding:0;} nav{background:#333; padding:15px; display:flex; justify-content:space-around; flex-wrap:wrap;} nav a{color:white; text-decoration:none; font-weight:bold; padding:5px;} section{background:white; padding:15px; margin:10px; border-radius:10px; box-shadow:0 2px 5px rgba(0,0,0,0.1);} input, select{width:100%; padding:10px; margin:5px 0; border:1px solid #ccc; border-radius:5px;} .mod-wrap{display:flex; gap:10px; justify-content: flex-start; padding: 5px 0;} .btn{background:#007bff; color:white; border:none; padding:15px; width:100%; border-radius:5px; font-weight:bold; cursor:pointer;} .label-row{font-weight:bold; display:block; margin-top:10px;} .hint{font-size:0.8em; color:#666; margin-bottom:10px;} .off-label{color:#d9534f; font-size:0.9em; font-weight:bold; margin-top:5px;}</style></head><body><nav><a href='/'>Keys</a><a href='/sw'>SW</a><a href='/rot'>Rot</a><a href='/scenes'>Szenen</a><a href='/hw'>Hardware</a></nav>"); 
}

void sendKeyOpts(AsyncResponseStream *res, uint16_t sel) { 
  for(int i=0; i<keyCount; i++) {
    res->printf("<option value='%u'%s>%s</option>", hidKeys[i].code, (sel==hidKeys[i].code?" selected":""), hidKeys[i].name); 
  }
  res->print("<optgroup label='Szenenwahl'>"); 
  for(int i=0; i<5; i++) {
    if(config.scenes[i].active) {
      res->printf("<option value='%d'%s>Gehe zu: %s</option>", 2000+i, (sel==(2000+i)?" selected":""), config.scenes[i].name); 
    }
  }
  res->print("</optgroup>"); 
}

void sendModCb(AsyncResponseStream *res, String n, uint8_t v) { 
  res->printf("<div class='mod-wrap'>");
  res->printf("<label><input type='checkbox' name='%s_c' value='1'%s> Strg</label> ", n.c_str(), (v & 1 ? " checked" : "")); 
  res->printf("<label><input type='checkbox' name='%s_s' value='2'%s> Shift</label> ", n.c_str(), (v & 2 ? " checked" : "")); 
  res->printf("<label><input type='checkbox' name='%s_a' value='4'%s> Alt</label> ", n.c_str(), (v & 4 ? " checked" : "")); 
  res->printf("<label><input type='checkbox' name='%s_g' value='8'%s> Win</label>", n.c_str(), (v & 8 ? " checked" : "")); 
  res->print("</div>");
}

void setupWebRoutes() {
  // --- SEITE 1: NUR TASTER ---
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *r) { 
    int sID = r->hasParam("s") ? r->getParam("s")->value().toInt() : 0; 
    AsyncResponseStream *res = r->beginResponseStream("text/html"); 
    res->print(getHead()); 
    res->printf("<section><label class='label-row'>Aktive Szene</label><select onchange='location.href=\"/?s=\"+this.value'>"); 
    for(int i=0; i<5; i++) res->printf("<option value='%d'%s>%s %s</option>", i, (sID==i?" selected":""), config.scenes[i].name, (config.scenes[i].active?"":"(Aus)")); 
    res->print(F("</select></section>")); 
    
    if(config.scenes[sID].active) { 
      res->printf("<form method='POST' action='/k-save?s=%d&from=keys'><section><h3>Taster: %s</h3>", sID, config.scenes[sID].name); 
      for(int b=0; b<8; b++) { 
        if(config.buttons[b].pinInput != -1 && !config.buttons[b].isSwitch) { 
          res->printf("<label class='label-row'>%s (Taster)</label>", config.buttons[b].label); 
          res->printf("<select name='b%dk'>", b); 
          sendKeyOpts(res, config.scenes[sID].btnK[b]); 
          res->print("</select>"); 
          sendModCb(res, "b"+String(b), config.scenes[sID].btnM[b]); 
          res->print("<hr>");
        } 
      } 
      res->print(F("<input type='submit' value='Taster Speichern' class='btn'></section></form>")); 
    } 
    res->print(F("</body></html>")); 
    r->send(res); 
  });

  // --- SEITE 2: NUR SCHALTER ---
  server.on("/sw", HTTP_GET, [](AsyncWebServerRequest *r) { 
    int sID = r->hasParam("s") ? r->getParam("s")->value().toInt() : 0; 
    AsyncResponseStream *res = r->beginResponseStream("text/html"); 
    res->print(getHead()); 
    res->printf("<section><label class='label-row'>Aktive Szene</label><select onchange='location.href=\"/sw?s=\"+this.value'>"); 
    for(int i=0; i<5; i++) res->printf("<option value='%d'%s>%s %s</option>", i, (sID==i?" selected":""), config.scenes[i].name, (config.scenes[i].active?"":"(Aus)")); 
    res->print(F("</select></section>")); 
    
    if(config.scenes[sID].active) { 
      res->printf("<form method='POST' action='/k-save?s=%d&from=sw'><section><h3>Schalter: %s</h3>", sID, config.scenes[sID].name); 
      for(int b=0; b<8; b++) { 
        if(config.buttons[b].pinInput != -1 && config.buttons[b].isSwitch) { 
          res->printf("<label class='label-row'>%s (Schalter)</label>", config.buttons[b].label); 
          res->printf("<select name='b%dk'>", b); 
          sendKeyOpts(res, config.scenes[sID].btnK[b]); 
          res->print("</select>"); 
          sendModCb(res, "b"+String(b), config.scenes[sID].btnM[b]); 
          
          res->printf("<div class='off-label'>Aktion bei AUS:</div><select name='b%dok'>", b);
          sendKeyOpts(res, config.scenes[sID].btnOffK[b]);
          res->print("</select>");
          sendModCb(res, "bo"+String(b), config.scenes[sID].btnOffM[b]);
          res->print("<hr>");
        } 
      } 
      res->print(F("<input type='submit' value='Schalter Speichern' class='btn'></section></form>")); 
    } 
    res->print(F("</body></html>")); 
    r->send(res); 
  });

  // --- SEITE 3: NUR DREHGEBER ---
  server.on("/rot", HTTP_GET, [](AsyncWebServerRequest *r) { 
    int sID = r->hasParam("s") ? r->getParam("s")->value().toInt() : 0; 
    AsyncResponseStream *res = r->beginResponseStream("text/html"); 
    res->print(getHead()); 
    res->printf("<section><label class='label-row'>Aktive Szene</label><select onchange='location.href=\"/rot?s=\"+this.value'>"); 
    for(int i=0; i<5; i++) res->printf("<option value='%d'%s>%s %s</option>", i, (sID==i?" selected":""), config.scenes[i].name, (config.scenes[i].active?"":"(Aus)")); 
    res->print(F("</select></section>")); 
    
    if(config.scenes[sID].active) { 
      res->printf("<form method='POST' action='/k-save?s=%d&from=rot'><section><h3>Drehgeber: %s</h3>", sID, config.scenes[sID].name); 
      const char* el[] = {"Enc L", "Enc R", "Enc L Fast", "Enc R Fast"}; 
      for(int e=0; e<4; e++) { 
        res->printf("<label class='label-row'>%s</label><select name='e%dk'>", el[e], e); 
        sendKeyOpts(res, config.scenes[sID].encK[e]); 
        res->print(F("</select>")); 
        sendModCb(res, "e"+String(e), config.scenes[sID].encM[e]); 
        res->print("<hr>");
      } 
      res->print(F("<input type='submit' value='Drehgeber Speichern' class='btn'></section></form>")); 
    } 
    res->print(F("</body></html>")); 
    r->send(res); 
  });

  // --- SAVE ROUTE ---
  server.on("/k-save", HTTP_POST, [](AsyncWebServerRequest *r) { 
    int s = r->hasParam("s") ? r->arg("s").toInt() : 0; 
    String from = r->arg("from");
    for(int b=0; b<8; b++) { 
      if(r->hasParam("b"+String(b)+"k", true)) {
        config.scenes[s].btnK[b] = r->arg("b"+String(b)+"k").toInt(); 
        uint8_t m=0; 
        if(r->hasParam("b"+String(b)+"_c",true)) m |= 1; 
        if(r->hasParam("b"+String(b)+"_s",true)) m |= 2; 
        if(r->hasParam("b"+String(b)+"_a",true)) m |= 4; 
        if(r->hasParam("b"+String(b)+"_g",true)) m |= 8;
        config.scenes[s].btnM[b]=m; 
      }
      if(r->hasParam("b"+String(b)+"ok", true)) {
        config.scenes[s].btnOffK[b] = r->arg("b"+String(b)+"ok").toInt();
        uint8_t mo=0;
        if(r->hasParam("bo"+String(b)+"_c",true)) mo |= 1; 
        if(r->hasParam("bo"+String(b)+"_s",true)) mo |= 2; 
        if(r->hasParam("bo"+String(b)+"_a",true)) mo |= 4; 
        if(r->hasParam("bo"+String(b)+"_g",true)) mo |= 8;
        config.scenes[s].btnOffM[b]=mo;
      }
    } 
    for(int e=0; e<4; e++) { 
      if(r->hasParam("e"+String(e)+"k", true)) {
        config.scenes[s].encK[e] = r->arg("e"+String(e)+"k").toInt(); 
        uint8_t m=0; 
        if(r->hasParam("e"+String(e)+"_c",true)) m |= 1; 
        if(r->hasParam("e"+String(e)+"_s",true)) m |= 2; 
        if(r->hasParam("e"+String(e)+"_a",true)) m |= 4; 
        if(r->hasParam("e"+String(e)+"_g",true)) m |= 8;
        config.scenes[s].encM[e]=m; 
      }
    } 
    saveConfig(); 
    if(from == "rot") r->redirect("/rot?s="+String(s));
    else if(from == "sw") r->redirect("/sw?s="+String(s));
    else r->redirect("/?s="+String(s)); 
  });

  // --- SZENEN & HARDWARE (Originalzustand wie gewünscht) ---
  server.on("/scenes", HTTP_GET, [](AsyncWebServerRequest *r) { 
    AsyncResponseStream *res = r->beginResponseStream("text/html"); 
    res->print(getHead()); 
    res->print(F("<form method='POST' action='/s-save'>")); 
    for(int s=0; s<5; s++) { 
      res->printf("<section><b>Szene %d</b><br>Aktiv: <input type='checkbox' name='ac%d' value='1'%s><br>Name: <input name='sn%d' value='%s' maxlength='16'></section>", s+1, s, (config.scenes[s].active?" checked":""), s, config.scenes[s].name); 
    } 
    res->print(F("<section><input type='submit' value='Speichern' class='btn'></section></form></body></html>")); 
    r->send(res); 
  });

  server.on("/s-save", HTTP_POST, [](AsyncWebServerRequest *r) { 
    for(int s=0; s<5; s++) { 
      config.scenes[s].active = r->hasParam("ac"+String(s), true); 
      strncpy(config.scenes[s].name, r->arg("sn"+String(s)).c_str(), 16); 
    } 
    saveConfig(); 
    r->redirect("/scenes"); 
  });

  server.on("/hw", HTTP_GET, [](AsyncWebServerRequest *r) { 
    AsyncResponseStream *res = r->beginResponseStream("text/html"); 
    res->print(getHead()); 
    res->print(F("<form method='POST' action='/h-save'><section><h3>Bluetooth Einstellungen</h3>"));
    res->printf("Name: <input name='btnm' value='%s' maxlength='31'>", config.deviceName);
    res->print(F("<p class='hint'>Hinweis: Nach Namensänderung Gerät am PC/Handy neu koppeln!</p></section>"));
    res->print(F("<section><h3>Hardware Encoder</h3>"));
    res->printf("Pin A/CLK: <input type='number' name='ea' value='%d'>", config.encA);
    res->printf("Pin B/DT: <input type='number' name='eb' value='%d'>", config.encB);
    res->printf("<br>Schritte/U: <input type='number' name='spr' value='%d'>", config.stepsPerRev);
    res->printf("<br>Schritte Fein: <input type='number' name='ss' value='%d'>", config.stepsSlow);
    res->printf("<br>Winkel/s Fast: <input type='number' name='st' value='%d'></section>", config.speedThreshold);

    for(int i=0; i<8; i++){ 
      res->printf("<section><b>Btn %d</b><br>Label: <input name='l%d' value='%s'> GPIO In: <input type='number' name='p%d' value='%d'> LED Out: <input type='number' name='led%d' value='%d'>", i+1, i, config.buttons[i].label, i, config.buttons[i].pinInput, i, config.buttons[i].pinLED); 
      res->printf("<br>Modus: <select name='m%d'><option value='0'%s>Taster</option><option value='1'%s>Schalter</option></select></section>", i, (config.buttons[i].isSwitch==0?" selected":""), (config.buttons[i].isSwitch==1?" selected":""));
    } 
    res->print(F("<section><input type='submit' value='Speichern und Reset' class='btn'></section></form><section><h3>OTA Update</h3><form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><br><br><input type='submit' value='Firmware Flash' class='btn' style='background:#28a745;'></form></section></body></html>")); 
    r->send(res); 
  });

  server.on("/h-save", HTTP_POST, [](AsyncWebServerRequest *r) { 
    if(r->hasParam("btnm", true)) strncpy(config.deviceName, r->arg("btnm").c_str(), 31);
    config.encA=r->arg("ea").toInt(); config.encB=r->arg("eb").toInt(); 
    config.stepsPerRev = r->arg("spr").toInt();
    config.stepsSlow = r->arg("ss").toInt();
    config.speedThreshold = r->arg("st").toInt();
    for(int i=0; i<8; i++){ 
      strncpy(config.buttons[i].label, r->arg("l"+String(i)).c_str(), 8); 
      config.buttons[i].pinInput=r->arg("p"+String(i)).toInt(); 
      config.buttons[i].pinLED=r->arg("led"+String(i)).toInt(); 
      config.buttons[i].isSwitch=r->arg("m"+String(i)).toInt(); 
    } 
    saveConfig(); 
    delay(500);
    ESP.restart(); 
  });

  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *r){ ESP.restart(); }, [](AsyncWebServerRequest *r, String fn, size_t idx, uint8_t *data, size_t len, bool final){ if(!idx){ if(!Update.begin(UPDATE_SIZE_UNKNOWN)) Update.printError(Serial); } if(!Update.hasError()){ if(Update.write(data, len) != len) Update.printError(Serial); } if(final){ if(Update.end(true)) Serial.printf("OK: %u\n", idx+len); else Update.printError(Serial); } });
  
  server.onNotFound([](AsyncWebServerRequest *request){
    request->redirect("/");
  });
}

void startWifiPortal() {
    configModeActive = true;
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(ssid, pass);
    dnsServer.start(53, "*", apIP);
    setupWebRoutes(); 
    server.begin();
}

void handlePortal() {
    dnsServer.processNextRequest();
}