#include "diagnostics_web.h"

#include <Preferences.h>

static String jsonEscape(const String& input) {
  String out;
  out.reserve(input.length() + 8);
  for (size_t i = 0; i < (size_t)input.length(); i++) {
    const char c = input[i];
    switch (c) {
      case '"': out += F("\\\""); break;
      case '\\': out += F("\\\\"); break;
      case '\n': out += F("\\n"); break;
      case '\r': out += F("\\r"); break;
      case '\t': out += F("\\t"); break;
      default: out += c; break;
    }
  }
  return out;
}

static uint8_t readPrefsU8(const char* ns, const char* key, uint8_t def) {
  if (ns == nullptr || key == nullptr) return def;
  Preferences prefs;
  if (!prefs.begin(ns, false)) return def;
  const uint8_t v = prefs.getUChar(key, def);
  prefs.end();
  return v;
}

static void ledsClear(CRGB* leds, int count) {
  if (leds == nullptr || count <= 0) return;
  for (int i = 0; i < count; i++) {
    leds[i] = CRGB::Black;
  }
}

static void ledsSetOne(CRGB* leds, int count, int idx, const CRGB& c) {
  if (leds == nullptr || count <= 0) return;
  if (idx < 0) idx = 0;
  if (idx >= count) idx = count - 1;
  for (int i = 0; i < count; i++) {
    leds[i] = (i == idx) ? c : CRGB::Black;
  }
}

static String buildDiagHtml() {
  String html;
  html.reserve(4096);
  html += F("<!doctype html><html><head><meta charset='utf-8'>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>KRONOS Diagnostics</title></head><body>");
  html += F("<h2>KRONOS Diagnostics</h2>");
  html += F("<p><a href='/'>Back to Config</a></p>");

  html += F("<h3>Live Readings</h3>");
  html += F("<pre id='j' style='white-space:pre-wrap'></pre>");

  html += F("<h3>LED Test</h3>");
  html += F("<div style='margin:10px 0'>");
  html += F("<label>Strip: ");
  html += F("<select id='strip'><option value='screen'>Screen (75)</option><option value='buttons'>Buttons (6)</option></select>");
  html += F("</label></div>");

  html += F("<div style='margin:10px 0'>");
  html += F("<label>Index: <input id='idx' type='number' min='0' value='0' style='width:80px'></label> ");
  html += F("<button onclick='nextLed()'>Next LED</button> ");
  html += F("<button onclick='clearLeds()'>Clear</button>");
  html += F("</div>");

  html += F("<div style='margin:10px 0'>");
  html += F("<label>Color: <input id='col' type='color' value='#ffffff'></label> ");
  html += F("<button onclick='setLed()'>Set LED</button>");
  html += F("</div>");

  html += F("<script>");
  html += F("function hexToRgb(h){h=h.replace('#','');return {r:parseInt(h.slice(0,2),16),g:parseInt(h.slice(2,4),16),b:parseInt(h.slice(4,6),16)};}");
  html += F("async function poll(){try{const r=await fetch('/api/diag');const j=await r.json();document.getElementById('j').textContent=JSON.stringify(j,null,2);}catch(e){document.getElementById('j').textContent='(error fetching /api/diag)';}setTimeout(poll,400);} poll();");
  html += F("async function post(url, body){await fetch(url,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});}");
  html += F("async function clearLeds(){const s=document.getElementById('strip').value;await post('/api/led/clear','strip='+encodeURIComponent(s));}");
  html += F("async function setLed(){const s=document.getElementById('strip').value;const idx=document.getElementById('idx').value||0;const rgb=hexToRgb(document.getElementById('col').value);const body='strip='+encodeURIComponent(s)+'&idx='+encodeURIComponent(idx)+'&r='+rgb.r+'&g='+rgb.g+'&b='+rgb.b;await post('/api/led/set',body);}");
  html += F("async function nextLed(){const el=document.getElementById('idx');el.value=parseInt(el.value||'0',10)+1;await setLed();}");
  html += F("</script>");

  html += F("</body></html>");
  return html;
}

static void writeDiagJson(WebServer& server, const DiagnosticsContext& ctx) {
  String json;
  json.reserve(2048);
  json += '{';

  json += F("\"millis\":");
  json += String((uint32_t)millis());

  json += F(",\"freeHeap\":");
  json += String((uint32_t)ESP.getFreeHeap());

  // Settings
  json += F(",\"settings\":{");
  const uint8_t meterStyle = readPrefsU8(ctx.prefsNamespace, "meterStyle", 0);
  const uint8_t ledBrightness = readPrefsU8(ctx.prefsNamespace, "ledBrightness", 20);
  json += F("\"meterStyle\":");
  json += String((int)meterStyle);
  json += F(",\"ledBrightness\":");
  json += String((int)ledBrightness);
  json += '}';

  // Physical button
  json += F(",\"buttonPin\":");
  json += String((int)ctx.physicalButtonPin);
  json += F(",\"buttonVal\":");
  json += String((int)digitalRead(ctx.physicalButtonPin));

  // Knob
  if (ctx.knob != nullptr) {
    json += F(",\"knob\":{");
    json += F("\"rawAngle\":");
    json += String((int)ctx.knob->readRawAngle());
    json += F(",\"mapped\":");
    json += String((int)ctx.knob->scanMapAngle(1024, 255, 1));
    json += '}';
  }

  // Hall sensors
  json += F(",\"hall\":[");
  for (size_t i = 0; i < ctx.hallCount; i++) {
    if (i) json += ',';
    MxgicHall* h = (ctx.hall != nullptr) ? ctx.hall[i] : nullptr;
    if (h == nullptr) {
      json += F("{\"idx\":");
      json += String((int)i);
      json += F(",\"err\":\"null\"}");
      continue;
    }

    const uint32_t raw = (uint32_t)h->rawRead();
    const bool pressed = (h->checkTrig(0) != 0);

    json += '{';
    json += F("\"idx\":");
    json += String((int)i);
    json += F(",\"adcSel\":");
    json += String((int)h->adcSel);
    json += F(",\"adcCh\":");
    json += String((int)h->adcCh);
    json += F(",\"raw\":");
    json += String((int)raw);
    json += F(",\"min\":");
    json += String((int)h->getMin());
    json += F(",\"max\":");
    json += String((int)h->getMax());
    json += F(",\"pressed\":");
    json += (pressed ? F("true") : F("false"));
    json += '}';
  }
  json += ']';

  // "Unused" pins probe list (safe, read-only)
  // Edit this list to match what you consider unused.
  static const uint8_t kProbePins[] = { 4, 5, 6, 7, 9, 10, 11, 12, 13, 14, 15, 16 };
  json += F(",\"probePins\":[");
  for (size_t i = 0; i < sizeof(kProbePins) / sizeof(kProbePins[0]); i++) {
    if (i) json += ',';
    const uint8_t p = kProbePins[i];
    json += '{';
    json += F("\"pin\":");
    json += String((int)p);
    json += F(",\"val\":");
    json += String((int)digitalRead(p));
    json += '}';
  }
  json += ']';

  json += '}';
  server.send(200, "application/json", json);
}

void diagnosticsWebRegisterRoutes(WebServer& server, const DiagnosticsContext& ctx) {
  // Diagnostics page
  server.on("/diag", HTTP_GET, [&server]() {
    server.send(200, "text/html", buildDiagHtml());
  });

  // JSON endpoint for live values
  server.on("/api/diag", HTTP_GET, [&server, ctx]() {
    writeDiagJson(server, ctx);
  });

  // LED controls
  server.on("/api/led/clear", HTTP_POST, [&server, ctx]() {
    const String strip = server.hasArg("strip") ? server.arg("strip") : "screen";
    if (strip == "buttons") {
      ledsClear(ctx.ledsButtons, ctx.ledsButtonsCount);
    } else {
      ledsClear(ctx.ledsScreen, ctx.ledsScreenCount);
    }
    FastLED.show();
    server.send(200, "text/plain", "OK");
  });

  server.on("/api/led/set", HTTP_POST, [&server, ctx]() {
    const String strip = server.hasArg("strip") ? server.arg("strip") : "screen";
    const int idx = server.hasArg("idx") ? server.arg("idx").toInt() : 0;
    const int r = server.hasArg("r") ? server.arg("r").toInt() : 255;
    const int g = server.hasArg("g") ? server.arg("g").toInt() : 255;
    const int b = server.hasArg("b") ? server.arg("b").toInt() : 255;

    CRGB c((uint8_t)constrain(r, 0, 255), (uint8_t)constrain(g, 0, 255), (uint8_t)constrain(b, 0, 255));

    if (strip == "buttons") {
      ledsSetOne(ctx.ledsButtons, ctx.ledsButtonsCount, idx, c);
    } else {
      ledsSetOne(ctx.ledsScreen, ctx.ledsScreenCount, idx, c);
    }

    FastLED.show();
    server.send(200, "text/plain", "OK");
  });
}
