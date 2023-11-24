#include "config.h"
#include "formatting.h"
#include "print.h"

#include <WiFi.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "Adafruit_Thermal.h"
#include <WebSocketsClient.h>
#include <HTTPClient.h>


#define TX_PIN 21 // Arduino transmit  YELLOW WIRE  labeled RX on printer
#define RX_PIN 2 // Arduino receive   GREEN WIRE   labeled TX on printer

HardwareSerial mySerial(1); // Declare HardwareSerial obj first
Adafruit_Thermal printer(&mySerial);     // Pass addr to printer constructor

WebSocketsClient webSocket;

bool hasReceiptToPrint = false;

JsonVariant lastPayment;

String currentBlockHeight = "818,114";

/**
 * @brief Get the current block height from https://mempool.space/api/blocks/tip/height formatted nicely with commas
 * This URL returns one line of text e.g. "832123"
 * 
 * @param type 
 * @param payload 
 * @param length 
 */
String getBlockHeight() {
  HTTPClient http;
  http.begin("https://mempool.space/api/blocks/tip/height");
  int httpCode = http.GET();
  String payload = http.getString();
  http.end();

  if (httpCode == 200) {
    Serial.println("Block height: " + payload);
    // format with commas
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

  // Extract the 'payment' object from the received JSON
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

  mySerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN); // Initialize serial
  printer.begin();        // Init printer (same regardless of serial type)
  printWelcomeReceipt();

  printTestReceipt();
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  // try to connect to WiFi for 10 seconds
  int i = 0;
  while (WiFi.status() != WL_CONNECTED && i < 10) {
    delay(1000);
    Serial.print(".");
    i++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to WiFi");
    printConnectedToWifi();
  } else {
    Serial.println("Failed to connect to WiFi");
    printFailedToConnectToWifi();
  }

  webSocket.beginSSL(host, 443, "/api/v1/ws/" + String(walletId));
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000); // Try to reconnect every 5 seconds
  webSocket.enableHeartbeat(5000, 3000, 2); // Send heartbeat every 15 seconds
}

long lastWebsocketPingTime = 0;

long lastBlockHeightRetrieved = 0;

void loop() {
  webSocket.loop();

  // get block height every 1 minute
  if (millis() - lastBlockHeightRetrieved > 60000) {
    currentBlockHeight = getBlockHeight();
    lastBlockHeightRetrieved = millis();
  }

  // ping websocket every 10 seconds
  if (millis() - lastWebsocketPingTime > 10000) {
    webSocket.sendPing();
    lastWebsocketPingTime = millis();
  }
  
  printReceipt();
}
