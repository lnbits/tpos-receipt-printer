#include "config.h"
#include <WiFi.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "Adafruit_Thermal.h"
#include <WebSocketsClient.h>


#define TX_PIN 15 // Arduino transmit  YELLOW WIRE  labeled RX on printer
#define RX_PIN 2 // Arduino receive   GREEN WIRE   labeled TX on printer

HardwareSerial mySerial(1); // Declare HardwareSerial obj first
Adafruit_Thermal printer(&mySerial);     // Pass addr to printer constructor

WebSocketsClient webSocket;

bool hasReceiptToPrint = false;

JsonVariant lastPayment;

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

void printConnectedToWifi() {
  String receipt = R"(

 Connected to WiFi
 $$ssid
************************


    )";
    receipt.replace("$$ssid", ssid);
    printer.wake();
    printer.print(receipt);
    printer.sleep();
}

void printWelcomeReceipt() {
  String receipt = R"(
************************
 LNbits
 TPoS Receipt Printer
 $$companyName
)";

    receipt.replace("$$companyName", companyName);
  
    printer.wake();
    printer.print(receipt);

   
    printer.sleep();
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


/**
 * @brief Convert a timestamp to Month, 20th 2021 at 12:00pm
 * 
 * @param timestamp 
 * @return String 
 */
String unixTimeStampToDateTime(long timestamp) {
  // Convert timestamp to Date
  struct tm * timeinfo;
  timeinfo = localtime(&timestamp);
  char date[20];
  strftime(date, 20, "%b %d, %Y", timeinfo);

  // Convert timestamp to Time
  char time[20];
  strftime(time, 20, "%H:%M", timeinfo);

  String dateTime = String(date) + " at " + String(time);
  return dateTime;
}

// print receipt
void printReceipt() {
  // if lastPayment is not empty

   String receipt = R"(

************************
  $$companyName
  PAYMENT RECEIVED
************************

$$time

$$memo

Amount: $$amount sats
Tip:    $$tip sats
----
Total:  $$total sats

************************
    Thank you!
************************


  )";

  if (hasReceiptToPrint) {
    Serial.println("Printing receipt...");

    String time = lastPayment["time"].as<String>();
    time = unixTimeStampToDateTime(time.toInt());

    String memo = lastPayment["memo"].as<String>();
    int amountMSats = lastPayment["amount"].as<int>();
    String tag = lastPayment["extra"]["tag"].as<String>();
    int tipAmount = lastPayment["extra"]["tipAmount"].as<int>();
    String paymentHash = lastPayment["payment_hash"].as<String>();
    int fee = lastPayment["fee"].as<int>();

    // print all the above to serial
    Serial.println("Time: " + time);
    Serial.println("Memo: " + memo);
    Serial.println("Amount: " + String(amountMSats));
    Serial.println("Tag: " + tag);
    Serial.println("Tip amount: " + String(tipAmount));
    Serial.println("Payment hash: " + paymentHash);
    Serial.println("Fee: " + String(fee));

    // search and replace in receipt
    receipt.replace("$$companyName", companyName);
    receipt.replace("$$time", time);
    receipt.replace("$$memo", memo);
    receipt.replace("$$amount", String(amountMSats / 1000 - tipAmount));
    receipt.replace("$$tip", String(tipAmount));
    receipt.replace("$$total", String(amountMSats / 1000));
    receipt.replace("$$fee", String(fee));


    // printer.doubleHeightOn();
    printer.wake();
    printer.print(receipt);
    printer.sleep();

    // printer.doubleHeightOff();
  }
  hasReceiptToPrint = false;
}


void setup() {
  Serial.begin(115200);
  Serial.println("Starting...");

  mySerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN); // Initialize serial
  printer.begin();        // Init printer (same regardless of serial type)
  printWelcomeReceipt();
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  printConnectedToWifi();

  webSocket.beginSSL(host, 443, "/api/v1/ws/" + String(walletId));
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000); // Try to reconnect every 5 seconds
  webSocket.enableHeartbeat(5000, 3000, 2); // Send heartbeat every 15 seconds
}

long lastWebsocketPingTime = 0;

void loop() {
  webSocket.loop();

  // ping websocket every 10 seconds
  if (millis() - lastWebsocketPingTime > 10000) {
    webSocket.sendPing();
    lastWebsocketPingTime = millis();
  }
  
  printReceipt();
}
