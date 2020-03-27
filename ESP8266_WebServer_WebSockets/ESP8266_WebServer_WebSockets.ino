#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WebSocketsServer.h>

#ifndef STASSID
#define STASSID "SegoviaWiFi"
#define STAPSK  "SegoviaFamily"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

void handleRoot() {
  server.send(200, "text/plain", "hello from esp8266!");
}

void handleNotFound() {
  server.send(404, "text/plain", "Page not found");
}

void setup(void) {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("jeffespserver")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);

  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("WebSocket started");
}

void loop(void) {
  server.handleClient();
  MDNS.update();
  webSocket.loop();
  sendFromSerial();
}


void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.print("Disconnected: ");
      Serial.println(num);
      break;
    case WStype_CONNECTED:
      Serial.print("Connected: ");
      Serial.println(num);
      break;
    case WStype_TEXT:
      displayMsg(payload, length);
      break;
  }
}

void displayMsg(uint8_t * payload, size_t len) {
  for (int i = 0; i < len; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void sendFromSerial() {
  if (Serial.available() > 0) {
    String s = Serial.readStringUntil('\n');
    unsigned int len = s.length() + 1;
    char c[len];
    s.toCharArray(c, len);
    webSocket.broadcastTXT(c, sizeof(c));
  }
}
