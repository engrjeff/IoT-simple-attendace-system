#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WebSocketsServer.h>

#include <SPI.h>
#include <MFRC522.h>

#ifndef STASSID
#define STASSID "SegoviaWiFi"
#define STAPSK  "SegoviaFamily"
#endif

#define RST_PIN         0          // D3 Configurable, see typical pin layout above
#define SS_PIN          2          // D4 Configurable, see typical pin layout above

ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

const char* ssid = STASSID;
const char* password = STAPSK;
byte readCard[4];

void setup() {
  initialize();
}

void loop() {
  runSystem();
}

void runSystem(){
  server.handleClient();
  MDNS.update();
  webSocket.loop();
  if (getID() == 1){
    char cardID[32] = "";
    byteArrToCharArr(readCard, 4, cardID);
    Serial.print("Card ID: ");
    Serial.println(cardID);
    webSocket.broadcastTXT(cardID, sizeof(cardID));
  }
}


void initialize() {
  Serial.begin(115200);   // Initialize serial communications with the PC
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  delay(4);       // Optional delay. Some board do need more time after init to be ready, see Readme

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

  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
  Serial.println(F("Ready to accept cards fro attendance.."));

}

uint8_t getID() {
  // Getting ready for Reading PICCs
  if ( ! mfrc522.PICC_IsNewCardPresent()) { //If a new PICC placed to RFID reader continue
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {   //Since a PICC placed get Serial and continue
    return 0;
  }

  for ( uint8_t i = 0; i < 4; i++) {  //
    readCard[i] = mfrc522.uid.uidByte[i];
//    Serial.print(readCard[i], HEX);
  }
  
  mfrc522.PICC_HaltA(); // Stop reading
  return 1;
}


// sample route handlers
void handleRoot() {
  server.send(200, "text/plain", "hello from esp8266!");
}

void handleNotFound() {
  server.send(404, "text/plain", "Page not found");
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

void byteArrToCharArr(byte array[], unsigned int len, char buffer[])
{
    for (unsigned int i = 0; i < len; i++)
    {
        byte nib1 = (array[i] >> 4) & 0x0F;
        byte nib2 = (array[i] >> 0) & 0x0F;
        buffer[i*2+0] = nib1  < 0xA ? '0' + nib1  : 'A' + nib1  - 0xA;
        buffer[i*2+1] = nib2  < 0xA ? '0' + nib2  : 'A' + nib2  - 0xA;
    }
    buffer[len*2] = '\0';
}

void displayMsg(uint8_t * payload, size_t len) {
  for (int i = 0; i < len; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}
