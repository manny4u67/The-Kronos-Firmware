#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

#include "wifi_config.h"
#include "keybinds.h"
#include "diagnostics_web.h"

static String htmlEscape(const String& input) {
  String out;
  out.reserve(input.length());
  for (size_t i = 0; i < (size_t)input.length(); i++) {
    const char c = input[i];
    switch (c) {
      case '&': out += F("&amp;"); break;
      case '<': out += F("&lt;"); break;
      case '>': out += F("&gt;"); break;
      case '"': out += F("&quot;"); break;
      default: out += c; break;
    }
  }
  return out;
}

static String buildConfigHtml(const String* actions, size_t actionCount, uint8_t meterStyle, uint8_t ledBrightness, bool enableDiagnostics) {
  String html;
  html.reserve(4600);
  html += F("<!doctype html><html><head><meta charset='utf-8'>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>KRONOS WiFi Config</title></head><body>");
  html += F("<h2>KRONOS Keybind Config</h2>");
  if (enableDiagnostics) {
    html += F("<p><a href='/diag'>Diagnostics</a></p>");
  }
  html += F("<p>Enter either <b>TYPE:</b>text to type, or a key combo like <b>CTRL+SHIFT+Z</b>, <b>GUI+NUM_MINUS</b>, <b>DELETE</b>.</p>");
  html += F("<form method='POST' action='/save'>");

  html += F("<div style='margin:10px 0'>");
  html += F("<label>Timer Meter Style: ");
  html += F("<select name='meterStyle'>");
  html += F("<option value='0'");
  if (meterStyle == 0) html += F(" selected");
  html += F(">White</option>");
  html += F("<option value='1'");
  if (meterStyle == 1) html += F(" selected");
  html += F(">Gradient</option>");
  html += F("</select>");
  html += F("</label></div>");

  html += F("<div style='margin:10px 0'>");
  html += F("<label>LED Brightness: ");
  html += F("<input id='b' name='ledBrightness' type='range' min='1' max='255' value='");
  html += String((int)ledBrightness);
  html += F("' oninput=\"document.getElementById('bv').value=this.value\"> ");
  html += F("<input id='bv' type='number' min='1' max='255' value='");
  html += String((int)ledBrightness);
  html += F("' oninput=\"document.getElementById('b').value=this.value; document.getElementById('b').dispatchEvent(new Event('input'))\"> ");
  html += F("</label></div>");

  for (size_t i = 0; i < actionCount; i++) {
    html += F("<div style='margin:10px 0'>");
    html += F("<label>");
    html += F("Button ");
    html += String((int)i + 1);
    html += F(": <input style='width:95%' maxlength='220' name='");
    html += keybindsKeyForButton(i);
    html += F("' value='");
    html += htmlEscape(actions[i]);
    html += F("'></label></div>");
  }
  html += F("<button type='submit'>Save & Reboot</button>");
  html += F("</form>");
  html += F("</body></html>");
  return html;
}

static void renderWifiConfigModeScreen(Adafruit_SSD1306& oled, const char* ssid, const IPAddress& ip) {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.println(F("WiFi Config Mode"));
  oled.println();
  oled.print(F("SSID: "));
  oled.println(ssid);
  oled.print(F("IP: "));
  oled.println(ip);
  oled.println();
  oled.println(F("Open / on phone"));
  oled.display();
}

void startWifiConfigPortal(const char* ssid,
                           const char* prefsNamespace,
                           Adafruit_SSD1306& oled,
                           String* actions,
                           size_t actionCount,
                           const DiagnosticsContext* diagCtx) {
  if (ssid == nullptr || prefsNamespace == nullptr || actions == nullptr || actionCount == 0) {
    return;
  }

  // Load existing (or initialize defaults) before serving UI.
  keybindsLoadFromPrefs(prefsNamespace, actions, actionCount);
  uint8_t meterStyle = keybindsLoadMeterStyleFromPrefs(prefsNamespace);
  uint8_t ledBrightness = keybindsLoadLedBrightnessFromPrefs(prefsNamespace);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid);

  IPAddress ip = WiFi.softAPIP();
  Serial.print(F("Config AP started. Connect to "));
  Serial.print(ssid);
  Serial.print(F(" then open http://"));
  Serial.println(ip);

  renderWifiConfigModeScreen(oled, ssid, ip);

  static WebServer server(80);

  if (diagCtx != nullptr) {
    diagnosticsWebRegisterRoutes(server, *diagCtx);
  }

  server.on("/", HTTP_GET, [actions, actionCount, meterStyle, ledBrightness, diagCtx]() {
    server.send(200, "text/html", buildConfigHtml(actions, actionCount, meterStyle, ledBrightness, diagCtx != nullptr));
  });

  server.on("/save", HTTP_POST, [prefsNamespace, actions, actionCount, meterStyle, ledBrightness]() mutable {
    if (server.hasArg("meterStyle")) {
      const String v = server.arg("meterStyle");
      meterStyle = (uint8_t)v.toInt();
      if (meterStyle > 1) meterStyle = 0;
    }

    if (server.hasArg("ledBrightness")) {
      const String v = server.arg("ledBrightness");
      int b = v.toInt();
      if (b < 1) b = 1;
      if (b > 255) b = 255;
      ledBrightness = (uint8_t)b;
    }

    for (size_t i = 0; i < actionCount; i++) {
      const String argName = keybindsKeyForButton(i);
      if (server.hasArg(argName)) {
        actions[i] = server.arg(argName);
        actions[i].trim();
      }
    }

    keybindsSaveToPrefs(prefsNamespace, actions, actionCount);
    keybindsSaveMeterStyleToPrefs(prefsNamespace, meterStyle);
    keybindsSaveLedBrightnessToPrefs(prefsNamespace, ledBrightness);

    server.send(200, "text/html", F("<html><body><h3>Saved. Rebooting...</h3></body></html>"));
    delay(500);
    ESP.restart();
  });

  server.begin();

  for (;;) {
    server.handleClient();
    delay(5);
  }
}
