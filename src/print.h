#ifndef PRINT_H
#define PRINT_H

#include "formatting.h"
#include "quotes.h"
#include "Adafruit_Thermal.h"
#include <WiFi.h>
#include <ArduinoJson.h>

extern Adafruit_Thermal printer;
extern bool hasReceiptToPrint;
extern JsonVariant lastPayment;
extern String currentBlockHeight;
extern const bool includeQuotes;

const char* wl_status_to_string(wl_status_t status) {
  switch (status) {
    case WL_NO_SHIELD: return "No WiFi shield is present";
    case WL_IDLE_STATUS: return "Idle";
    case WL_NO_SSID_AVAIL: return "No SSIDs are available";
    case WL_SCAN_COMPLETED: return "Network scan completed";
    case WL_CONNECTED: return "Connected to a network";
    case WL_CONNECT_FAILED: return "Connection failed";
    case WL_CONNECTION_LOST: return "Connection lost";
    case WL_DISCONNECTED: return "Disconnected from the network";
  }
  return "UNKNOWN";
}

void printFailedToConnectToWifi() {
      String receipt = R"(
 Failed to connect to WiFi
 $$ssid: $$error


)";
    String error = wl_status_to_string(WiFi.status());
    receipt.replace("$$ssid", ssid);
    receipt.replace("$$error", error);

    Serial.print(receipt);

    printer.wake();
    printer.print(receipt);
    printer.sleep();
}

void printConnectedToWifi() {
  String receipt = R"(
 Connected to $$ssid


    )";
    Serial.print(receipt);
    receipt.replace("$$ssid", ssid);
    printer.wake();
    printer.print(receipt);
    printer.sleep();
}

void printWelcomeReceipt() {
  String receipt = R"(

 $$companyName
)";

    receipt.replace("$$companyName", companyName);
    Serial.print(receipt);
    printer.wake();
    printer.print(receipt);
    printer.sleep();
}

/**
 * @brief Print the receipt
 * 
 */
void printReceipt() {
  // if lastPayment is not empty

   String receipt = R"(

  $$companyName
  Sales Receipt
------------------------

$$time
Block Height: $$blockHeight

Amount: $$amount sats
Tip:    $$tip sats

Total:  $$total sats
Total:  $$fiatTotal

Payment Hash:
$$paymentHash
------------------------
    Thank you!
------------------------
  )";

  if (hasReceiptToPrint) {

    String time = lastPayment["time"].as<String>();
    time = unixTimeStampToDateTime(time.toInt());

    String memo = lastPayment["memo"].as<String>();
    String fiatTotal = memo.substring(0, memo.indexOf(" "));
    int amountMSats = lastPayment["amount"].as<int>();
    String tag = lastPayment["extra"]["tag"].as<String>();
    int tipAmount = lastPayment["extra"]["tipAmount"].as<int>();
    String paymentHash = lastPayment["payment_hash"].as<String>();
    int fee = lastPayment["fee"].as<int>();

    // search and replace in receipt
    receipt.replace("$$companyName", companyName);
    receipt.replace("$$time", time);
    receipt.replace("$$memo", memo);
    receipt.replace("$$amount", formatNumber(amountMSats / 1000 - tipAmount));
    receipt.replace("$$tip", formatNumber(tipAmount));
    receipt.replace("$$total", formatNumber(amountMSats / 1000));
    receipt.replace("$$fiatTotal", fiatTotal);
    receipt.replace("$$fee", String(fee));
    receipt.replace("$$paymentHash", paymentHash);
    receipt.replace("$$blockHeight", currentBlockHeight);

    if(includeQuotes) {
        String quote = getRandomQuote();
        receipt += "\n" + quote;
    }
    receipt += "\n\n";

    Serial.print(receipt);

    printer.wake();
    printer.print(receipt);
    printer.sleep();
  }
  hasReceiptToPrint = false;
}

void printTestReceipt() {
  // create a JsonVariant test of lastPayment for testing, it needs:
  // time, memo, amount, extra.tag, extra.tipAmount, payment_hash, fee
    DynamicJsonDocument doc(4096);
    doc["time"] = 1626796800;
    doc["memo"] = "$21.00 to Test TPoS";
    doc["amount"] = 21000000;
    doc["extra"]["tag"] = "tpos";
    doc["extra"]["tipAmount"] = 210;
    // example lightning payment hash
    doc["payment_hash"] = "0001020304050607080900010203040506070809000102030405060708090102";
    doc["fee"] = 0;
    lastPayment = doc.as<JsonVariant>();
    hasReceiptToPrint = true;
    printReceipt();
    // clear lastPayment
    lastPayment = JsonVariant();
}

#endif