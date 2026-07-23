#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

#include "Lightbar.h"

// ---- Configuration -- edit these ------------------------------------------
#define WIFI_SSID "ssid"
#define WIFI_PASS "pass"

// 3-byte id of your Xiaomi remote. Capture it with
// scripts/scan_lightbar_remote.py, or use an arbitrary id and re-pair the bar
// (unplug/replug the bar, then trigger within 20 s).
#define REMOTE_ID 0x45b510

// nRF24L01 control pins (SPI pins SCK=4, MISO=5, MOSI=6 are set in Lightbar.cpp).
// Overridable via build_flags so each board env can supply its own wiring.
#ifndef NRF_CE_PIN
#define NRF_CE_PIN 10
#endif
#ifndef NRF_CSN_PIN
#define NRF_CSN_PIN 7
#endif

// Set to 1 to print the gold-vector self-test on boot (no hardware needed).
#define PACKET_SELFTEST 1
// ---------------------------------------------------------------------------

Lightbar lightbar(NRF_CE_PIN, NRF_CSN_PIN, REMOTE_ID);
WebServer server(80);

static const char INDEX_HTML[] PROGMEM =
    "<!doctype html><html><head><meta name=viewport "
    "content='width=device-width,initial-scale=1'><title>Xiaomi "
    "Lightbar</title></head><body style='font-family:sans-serif;text-align:"
    "center;margin-top:20vh'><h1>Xiaomi Lightbar</h1>"
    "<button style='font-size:2rem;padding:1rem 2rem' "
    "onclick=\"fetch('/on_off')\">On / Off</button></body></html>";

void handleRoot() { server.send_P(200, "text/html", INDEX_HTML); }

void handleOnOff() {
  lightbar.on_off();
  Serial.println("on_off sent");
  server.send(200, "text/plain", "ok");
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();

#if PACKET_SELFTEST
  // With REMOTE_ID = 0xABCDEF this must print
  // 533914DD1C493412ABCDEFFF720100FAD4
  Serial.print("Self-test packet: ");
  Serial.println(lightbar.packetHex(0x0100, 0x72));
#endif

  if (lightbar.begin()) {
    Serial.println("nRF24 chip connected");
  } else {
    Serial.println("WARNING: nRF24 chip NOT responding -- check wiring/power");
  }

  Serial.printf("Connecting to WiFi '%s'", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected. IP: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/on_off", handleOnOff);
  server.begin();
  Serial.println("HTTP server started -- GET /on_off to toggle");
}

void loop() { server.handleClient(); }
