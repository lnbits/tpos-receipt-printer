#include <WiFi.h>
#include <SPIFFS.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Adafruit_Thermal.h"

#define TX_PIN 15 // Arduino transmit  YELLOW WIRE  labeled RX on printer
#define RX_PIN 2 // Arduino receive   GREEN WIRE   labeled TX on printer

HardwareSerial mySerial(1); // Declare HardwareSerial obj first
Adafruit_Thermal printer(&mySerial);     // Pass addr to printer constructor

const char* ssid = "PLUSNET-MWC9Q2";
const char* password = "4NyMeXtNcQ6rqP";

const char* apiUrl = "https://legend.lnbits.com/api/v1/payments/paginated?limit=10&offset=0&sortby=time&direction=desc";
const char* apiKey = "a5e68ac0d7f64711a863c5195f664687"; // Replace with your actual API key

String lastPaymentHash = "";
bool hasReceiptToPrint = false;

JsonVariant lastPayment;


void makeHttpRequest() {
  HTTPClient http;
  http.begin(apiUrl);
  http.addHeader("X-API-Key", apiKey);

  int httpResponseCode = http.GET();
  
  if (httpResponseCode > 0) {
    String payload = http.getString();
    deserializeAndCompare(payload);
  } else {
    Serial.print("Error on HTTP request: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

void deserializeAndCompare(String json) {
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, json);

  // loop around the doc["data"] item
  for (JsonVariant value : doc["data"].as<JsonArray>()) {
    String paymentHash = value["payment_hash"].as<String>();
    Serial.println("Payment hash: " + paymentHash);
    String tag = value["extra"]["tag"].as<String>();
    Serial.println("Tag: " + tag);
    if (paymentHash != lastPaymentHash) {
      Serial.println("New payment received!");
      Serial.println(paymentHash);
      lastPaymentHash = paymentHash;
      lastPayment = value;
      hasReceiptToPrint = true;

      // Save last payment hash to SPIFFS
      File file = SPIFFS.open("/lastPaymentHash.txt", FILE_WRITE);
      if (!file) {
        Serial.println("Failed to open file for writing");
        return;
      }
      if (file.print(paymentHash)) {
        Serial.println("File written");
      } else {
        Serial.println("Write failed");
      }
      file.close();
    }
  }
}

// print receipt
void printReceipt() {
  // if lastPayment is not empty
  if (hasReceiptToPrint) {
    Serial.println("Printing receipt...");
    Serial.println(lastPayment["payment_hash"].as<String>());
    Serial.println(lastPayment["memo"].as<String>());
    Serial.println(lastPayment["extra"]["tag"].as<String>());

    printer.doubleHeightOn();
    printer.println(F("PRINT!!!"));
    printer.doubleHeightOff();
  }
  hasReceiptToPrint = false;
}


void setup() {
  Serial.begin(115200);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  mySerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN); // Initialize serial
  printer.begin();        // Init printer (same regardless of serial type)


  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Read last payment hash
  File file = SPIFFS.open("/lastPaymentHash.txt", FILE_READ);
  if (file) {
    // lastPaymentHash = file.readString();
    file.close();
  }
}

void loop() {
  makeHttpRequest();
  printReceipt();
  delay(5000);
}
