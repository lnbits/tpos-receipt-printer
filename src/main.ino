#include "config.h"
#include "formatting.h"
#include "print.h"

#include <WiFi.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "Adafruit_Thermal.h"
#include <WebSocketsClient.h>
#include <HTTPClient.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TX_PIN 15 // Arduino transmit  YELLOW WIRE  labeled RX on printer
#define RX_PIN 2 // Arduino receive   GREEN WIRE   labeled TX on printer

HardwareSerial mySerial(1); // Declare HardwareSerial obj first
Adafruit_Thermal printer(&mySerial);     // Pass addr to printer constructor

WebSocketsClient webSocket;

long lastWebsocketPingTime = 0;

QueueHandle_t paymentQueue;

String currentBlockHeight = "";

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
  JsonObject paymentObj = doc["payment"];

  if (!paymentObj.isNull()) {

    long time = paymentObj["time"].as<long>();
    String memo = paymentObj["memo"].as<String>();
    int amountMSats = paymentObj["amount"].as<int>();
    String tag = paymentObj["extra"]["tag"].as<String>();
    int tipAmount = paymentObj["extra"]["tipAmount"].as<int>();
    String paymentHash = paymentObj["payment_hash"].as<String>();
    int fee = paymentObj["fee"].as<int>();
    bool pending = paymentObj["pending"].as<bool>();

    // serial print all the things
    Serial.println("time: " + String(time));
    Serial.println("memo: " + memo);
    Serial.println("amountMSats: " + String(amountMSats));
    Serial.println("tag: " + tag);
    Serial.println("tipAmount: " + String(tipAmount));
    Serial.println("paymentHash: " + paymentHash);
    Serial.println("fee: " + String(fee));
    Serial.println("pending: " + String(pending));
    

    if (!pending && tag == "tpos" && memo.indexOf("tip") == -1) {
      Serial.println("New payment received!");

      Payment newPayment;
      newPayment.time = time;
      newPayment.memo = memo;
      newPayment.amount = amountMSats;
      newPayment.tipAmount = tipAmount;
      newPayment.tag = tag;
      newPayment.paymentHash = paymentHash;      

      if (!xQueueSend(paymentQueue, &newPayment, portMAX_DELAY)) {
        Serial.println("Failed to send payment to the queue");
      }
    }
  }
}

void websocketTask(void * parameter) {
  Serial.println("websocketTask niit");
  for(;;) {
    Serial.println("websocketTask");
    webSocket.loop();

    if (millis() - lastWebsocketPingTime > 10000) {
      webSocket.sendPing();
      lastWebsocketPingTime = millis();
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

// printReceipt freertos task
void printReceiptTask(void * parameter) {
  for(;;) {
    printReceipt();
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

// BlockHeight freertos task
void getBlockHeightTask(void * parameter) {
  Serial.println("getBlockHeightTask");
  for(;;) {
    currentBlockHeight = getBlockHeight();
    vTaskDelay(60000 / portTICK_PERIOD_MS);
  }
}

// freertos task setup
void setupTasks() {
  Serial.println("setupTasks");
  // printReceiptTask
  // xTaskCreatePinnedToCore(
  //   printReceiptTask,   /* Task function. */
  //   "printReceiptTask", /* name of task. */
  //   1000,               /* Stack size of task */
  //   NULL,               /* parameter of the task */
  //   0,                  /* priority of the task */
  //   NULL,               /* Task handle to keep track of created task */
  //   0);                 /* pin task to core 0 */

  // xTaskCreatePinnedToCore(
  //   getBlockHeightTask,   /* Task function. */
  //   "getBlockHeightTask", /* name of task. */
  //   1000,                /* Stack size of task */
  //   NULL,                 /* parameter of the task */
  //   0,                    /* priority of the task */
  //   NULL,                 /* Task handle to keep track of created task */
  //   1);                   /* pin task to core 0 */ 

  xTaskCreatePinnedToCore(
    websocketTask,   /* Task function. */
    "websocketTask", /* name of task. */
    2000,           /* Stack size of task */
    NULL,            /* parameter of the task */
    0,               /* priority of the task */
    NULL,            /* Task handle to keep track of created task */
    0);              /* pin task to core 0 */
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting...");

  mySerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN); // Initialize serial
  printer.begin();        // Init printer (same regardless of serial type)
  printWelcomeReceipt();

  // printTestReceipt();
  
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
  
  // create the queue
  paymentQueue = xQueueCreate(10, sizeof(Payment));

  setupTasks();
}

void loop() {
  
}
