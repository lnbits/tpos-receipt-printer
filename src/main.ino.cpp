# 1 "/var/folders/96/v_l40xh56t74ymwrdnrq84d80000gn/T/tmpnatg6l4t"
#include <Arduino.h>
# 1 "/Users/mark/projects/lnbits/LNbits-TPoS-Printer/src/main.ino"
#include "config.h"
#include "formatting.h"
#include "print.h"

#include <WiFi.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "Adafruit_Thermal.h"
#include <WebSocketsClient.h>
#include <HTTPClient.h>


#define TX_PIN 15
#define RX_PIN 2

HardwareSerial mySerial(1);
Adafruit_Thermal printer(&mySerial);

WebSocketsClient webSocket;

bool hasReceiptToPrint = false;

JsonVariant lastPayment;

String currentBlockHeight = "";
# 35 "/Users/mark/projects/lnbits/LNbits-TPoS-Printer/src/main.ino"
String getBlockHeight();
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
void deserializeAndCompare(String json);
void setup();
void loop();
#line 35 "/Users/mark/projects/lnbits/LNbits-TPoS-Printer/src/main.ino"
String getBlockHeight() {
  HTTPClient http;
  http.begin("https://mempool.space/api/blocks/tip/height");
  int httpCode = http.GET();
  String payload = http.getString();
  http.end();

  if (httpCode == 200) {
    Serial.println("Block height: " + payload);

    int blockHeight = payload.toInt();

    return formatNumber(blockHeight);
  } else {
    Serial.println("Error getting block height");
    return "";
  }
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[WebSocket] Disconnected!\n");
      break;
    case WStype_CONNECTED:
      Serial.printf("[WebSocket] Connected to url: %s\n", payload);
      break;
    case WStype_TEXT:
      Serial.printf("[WebSocket] Received text: %s\n", payload);
      deserializeAndCompare(String((char *)payload));
      break;
  }
}

void deserializeAndCompare(String json) {
  DynamicJsonDocument doc(4096);
  deserializeJson(doc, json);


  JsonObject payment = doc["payment"];

  if (!payment.isNull()) {
    long time = payment["time"].as<long>();
    String paymentHash = payment["payment_hash"].as<String>();
    Serial.println("Payment time: " + String(time));
    String tag = payment["extra"]["tag"].as<String>();
    Serial.println("Tag: " + tag);
    String memo = payment["memo"].as<String>();
    Serial.println("Memo: " + memo);
    bool pending = payment["pending"].as<bool>();

    if (!pending && tag == "tpos" && memo.indexOf("tip") == -1) {
      Serial.println("New payment received!");
      lastPayment = payment;
      hasReceiptToPrint = true;
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting...");

  mySerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  printer.begin();
  printWelcomeReceipt();




  WiFi.begin(ssid, password);

  int i = 0;
  while (WiFi.status() != WL_CONNECTED && i < 10) {
    delay(1000);
    Serial.print(".");
    i++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to WiFi");
    printConnectedToWifi();
    return;
  } else {
    Serial.println("Failed to connect to WiFi");
    printFailedToConnectToWifi();
  }

  webSocket.beginSSL(host, 443, "/api/v1/ws/" + String(walletId));
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
  webSocket.enableHeartbeat(5000, 3000, 2);
}

long lastWebsocketPingTime = 0;

long lastBlockHeightRetrieved = 0;

void loop() {
  webSocket.loop();


  if (millis() - lastBlockHeightRetrieved > 60000) {
    currentBlockHeight = getBlockHeight();
    lastBlockHeightRetrieved = millis();
  }


  if (millis() - lastWebsocketPingTime > 10000) {
    webSocket.sendPing();
    lastWebsocketPingTime = millis();
  }

  printReceipt();
}