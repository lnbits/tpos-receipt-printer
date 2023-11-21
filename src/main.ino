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

long lastPrintedPaymentTime = 0;
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
    Welcome to LNbits 
    )";
  
    printer.wake();
    printer.print(receipt);
    printer.sleep();
  }

void deserializeAndCompare(String json) {
  DynamicJsonDocument doc(4096);
  deserializeJson(doc, json);

  // loop around the doc["data"] item
  for (JsonVariant value : doc["data"].as<JsonArray>()) {
    long time = value["time"].as<long>();
    String paymentHash = value["payment_hash"].as<String>();
    Serial.println("Payment time: " + String(time));
    String tag = value["extra"]["tag"].as<String>();
    Serial.println("Tag: " + tag);
    String memo = value["memo"].as<String>();
    Serial.println("Memo: " + memo);
    String pending = value["pending"].as<String>();

    if (
      pending == "false"
      &&
      tag == "tpos"
      &&
      memo.indexOf("tip") == -1
      && 
      time > lastPrintedPaymentTime
    ) {
      lastPrintedPaymentTime = time;
      Serial.println("New payment received!");
      lastPayment = value;
      hasReceiptToPrint = true;

      // Save last payment hash to SPIFFS
      File file = SPIFFS.open("/lastPaymentTime.txt", FILE_WRITE);
      if (!file) {
        Serial.println("Failed to open file for writing");
        return;
      }
      if (file.print(lastPrintedPaymentTime)) {
        Serial.println("File written");
      } else {
        Serial.println("Write failed");
      }
      file.close();
      break;
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
  strftime(date, 20, "%b %d %Y", timeinfo);

  // Convert timestamp to Time
  char time[20];
  strftime(time, 20, "%I:%M%p", timeinfo);

  String dateTime = String(date) + " at " + String(time);
  return dateTime;
}

// print receipt
void printReceipt() {
  // if lastPayment is not empty

   String receipt = R"(


************************
    PAYMENT RECEIVED
************************

$$time

$$memo

Amount: $$amount sats
Tip:    $$tip sats

Total:  $$total sats

Fee:   $$fee sats

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
    receipt.replace("$$time", time);
    receipt.replace("$$memo", memo);
    receipt.replace("$$amount", String(amountMSats / 1000));
    receipt.replace("$$tip", String(tipAmount));
    receipt.replace("$$total", String(amountMSats / 1000 + tipAmount));
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


  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Read last payment hash
  File file = SPIFFS.open("/lastPaymentTime.txt", FILE_READ);
  if (file) {
    String lastPrintedPaymentTimeString = file.readString();
    lastPrintedPaymentTime = lastPrintedPaymentTimeString.toInt();
    Serial.println("Last payment time: " + lastPrintedPaymentTimeString);
    file.close();
  }
}

void loop() {
  makeHttpRequest();
  printReceipt();
  delay(10000);
}
