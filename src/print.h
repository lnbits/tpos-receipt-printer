#ifndef PRINT_H
#define PRINT_H

#include "formatting.h"
#include "quotes.h"
#include "Adafruit_Thermal.h"
#include <WiFi.h>
#include <ArduinoJson.h>

extern Adafruit_Thermal printer;
extern String currentBlockHeight;
extern const bool includeQuotes;
extern QueueHandle_t paymentQueue;

struct Payment {
  long time;
  String memo;
  int amount;
  String tag;
  int tipAmount;
  String paymentHash;
  int fee;
};

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
extern QueueHandle_t paymentQueue;  // Ensure this is declared globally

void printReceipt() {
    Payment payment;
    // Wait for a payment to be available in the queue
    if (xQueueReceive(paymentQueue, &payment, portMAX_DELAY)) {

        // String receipt = R"(

        // $$companyName
        // Sales Receipt
        // ------------------------

        // $$time
        // Block Height: $$blockHeight

        // Amount: $$amount sats
        // Tip:    $$tip sats

        // Total:  $$total sats
        // Total:  $$fiatTotal

        // Payment Hash:
        // $$paymentHash
        // ------------------------
        //     Thank you!
        // ------------------------
        // )";

        // // Convert Unix timestamp to DateTime format
        // String time = unixTimeStampToDateTime(payment.time);

        // // Extract fiat total from memo (assuming it's stored at the beginning of the memo)
        // String fiatTotal = payment.memo.substring(0, payment.memo.indexOf(" "));

        // // Prepare data for replacement
        // receipt.replace("$$companyName", companyName);
        // receipt.replace("$$time", time);
        // receipt.replace("$$memo", payment.memo);
        // receipt.replace("$$amount", formatNumber(payment.amount / 1000 - payment.tipAmount));
        // receipt.replace("$$tip", formatNumber(payment.tipAmount));
        // receipt.replace("$$total", formatNumber(payment.amount / 1000));
        // receipt.replace("$$fiatTotal", fiatTotal);
        // receipt.replace("$$paymentHash", payment.paymentHash);
        // receipt.replace("$$blockHeight", currentBlockHeight);

        // if(includeQuotes) {
        //     String quote = getRandomQuote();
        //     receipt += "\n" + quote;
        // }
        // receipt += "\n\n";

        // Serial.print(receipt);

        // printer.wake();
        // printer.print(receipt);
        // printer.sleep();
    }
}

void printTestReceipt() {
  // push a payment to the queue
  Payment payment;
  payment.time = 1620000000;
  payment.memo = "$21.00 to TPoS demo";
  payment.amount = 21000000;
  payment.tipAmount = 210;
  payment.tag = "tpos";
  payment.paymentHash = "0001020304050607080900010203040506070809000102030405060708090102";
  payment.fee = 0;
  if (!xQueueSend(paymentQueue, &payment, portMAX_DELAY)) {
    Serial.println("Failed to send payment to the queue");
  }
}

#endif