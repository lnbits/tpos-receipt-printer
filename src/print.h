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

const char *wl_status_to_string(wl_status_t status)
{
  switch (status)
  {
  case WL_NO_SHIELD:
    return "No WiFi shield is present";
  case WL_IDLE_STATUS:
    return "Idle";
  case WL_NO_SSID_AVAIL:
    return "No SSIDs are available";
  case WL_SCAN_COMPLETED:
    return "Network scan completed";
  case WL_CONNECTED:
    return "Connected to a network";
  case WL_CONNECT_FAILED:
    return "Connection failed";
  case WL_CONNECTION_LOST:
    return "Connection lost";
  case WL_DISCONNECTED:
    return "Disconnected from the network";
  }
  return "UNKNOWN";
}

void printFailedToConnectToWifi()
{
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

void printConnectedToWifi()
{
  String receipt = R"(
 Connected to $$ssid


    )";
  Serial.print(receipt);
  receipt.replace("$$ssid", ssid);
  printer.wake();
  printer.print(receipt);
  printer.sleep();
}

void printWelcomeReceipt()
{
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
 * @brief Get a string of payment items from the extra.details JSON string
 *
 * @param paymentExtraDetails
 */
String getPaymentItems(String paymentExtraDetails)
{
  Serial.println("Payment extra details: " + paymentExtraDetails);
  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, paymentExtraDetails);
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return "";
  }
  JsonArray items;
  try
  {
    items = doc["items"].as<JsonArray>();
  }
  catch (const std::exception &e)
  {
    Serial.println("Error getting items from extra.details");
    Serial.println(e.what());
    return "";
  }
  String itemsString = "";
  // loop through items and add to itemsString using title, quantity and formattedPrice
  for (JsonVariant item : items)
  {
    String title = item["title"].as<String>();
    int quantity = item["quantity"].as<int>();
    String formattedPrice = item["formattedPrice"].as<String>();
    itemsString += title + " x " + String(quantity) + "   " + formattedPrice + "\n";
  }
  Serial.println("Items: " + itemsString);
  return itemsString;
}

/**
 * @brief Print the receipt
 *
 */
void printReceipt()
{
  // if lastPayment is not empty

  String receipt = R"(

$$companyName
Sales Receipt
------------------------
$$time
Block Height: $$blockHeight
$$items
Amount: $$amount sats $$subTotalFiat
Tip:    $$tip sats $$tipsFiat

Total: $$total sats
Total: $$walletFiatAmount $$walletFiatCurrency

Payment Hash:
$$paymentHash
------------------------
Thank you!
1 BTC = $$fiatExchangeRate $$walletFiatCurrency
------------------------
)";

  if (hasReceiptToPrint)
  {

    String time = lastPayment["time"].as<String>();
    time = unixTimeStampToDateTime(time.toInt());

    String memo = lastPayment["memo"].as<String>();
    String fiatTotal = memo.substring(0, memo.indexOf(" "));
    int amountMSats = lastPayment["amount"].as<int>();

    String tag = lastPayment["extra"]["tag"].as<String>();
    int tipAmount = lastPayment["extra"]["tipAmount"].as<int>();
    String paymentHash = lastPayment["payment_hash"].as<String>();
    int fee = lastPayment["fee"].as<int>();
    float walletFiatAmount = 0;
    String walletFiatCurrency = "";

    if (lastPayment["extra"]["wallet_fiat_amount"].is<float>())
    {
      walletFiatAmount = lastPayment["extra"]["wallet_fiat_amount"].as<float>();
      fiatTotal = String(walletFiatAmount, 2);
    }
    if (lastPayment["extra"]["wallet_fiat_currency"].is<String>())
    {
      walletFiatCurrency = lastPayment["extra"]["wallet_fiat_currency"].as<String>();
    }
    // if extras.details exists, get the itemsDetail
    String itemsDetail = "";
    if (lastPayment["extra"]["details"].is<String>())
    {
      Serial.println("Getting items from extra.details" + lastPayment["extra"]["details"].as<String>());
      itemsDetail = "\r\n" + getPaymentItems(lastPayment["extra"]["details"].as<String>());
    }

    // if extra.wallet_fiat_rate exists, convert to currency per bitcoin
    String fiatExchangeRate = "";
    float walletFiatRate = 0;
    if (lastPayment["extra"]["wallet_fiat_rate"].is<float>())
    {
      walletFiatRate = lastPayment["extra"]["wallet_fiat_rate"].as<float>();
      // this is sats per currency unit, convert to currency per bitcoin (100,000,000 sats)
      float fiatExchangeRateFloat = 100000000 / walletFiatRate;
      fiatExchangeRate = formatNumber(fiatExchangeRateFloat);
    }
    // if we have a walletFiatRate (sats per currency unit), create a subTotalMSats fiat value
    String subTotalFiat = "";
    String tipsFiat = "";
    if (walletFiatRate > 0)
    {
      float subTotalFiatFloat = (amountMSats / 1000 - tipAmount) / walletFiatRate;
      Serial.println("subTotalFiatFloat: " + String(subTotalFiatFloat));
      subTotalFiat = String(subTotalFiatFloat, 2);
      subTotalFiat += " " + walletFiatCurrency;
      // same for tips
      float tipsFiatFloat = tipAmount / walletFiatRate;
      tipsFiat = String(tipsFiatFloat, 2);
      tipsFiat += " " + walletFiatCurrency;
    }

    // search and replace in receipt
    receipt.replace("$$companyName", companyName);
    receipt.replace("$$time", time);
    receipt.replace("$$memo", memo);
    receipt.replace("$$items", itemsDetail);
    receipt.replace("$$amount", formatNumber(amountMSats / 1000 - tipAmount));
    receipt.replace("$$subTotalFiat", " / " + subTotalFiat);
    receipt.replace("$$tip ", formatNumber(tipAmount));
    receipt.replace("$$tipsFiat", " / " + tipsFiat);
    receipt.replace("$$total", formatNumber(amountMSats / 1000));
    receipt.replace("$$walletFiatAmount", fiatTotal);
    receipt.replace("$$walletFiatCurrency", walletFiatCurrency);
    receipt.replace("$$fee", String(fee));
    receipt.replace("$$paymentHash", paymentHash);
    receipt.replace("$$blockHeight", currentBlockHeight);
    receipt.replace("$$fiatExchangeRate", fiatExchangeRate);

    if (includeQuotes)
    {
      String quote = getRandomQuote();
      // add two blank lines
      receipt += quote + "\r\n\r\n\r\n\r\n";
    }

    Serial.print(receipt);

    printer.wake();
    printer.print(receipt);

    printer.sleep();
  }
  hasReceiptToPrint = false;
}

void printTestReceipt()
{
  String paymentJson = R"(
    {
    "checking_id":"93aa22184ed9e0c577dc8b67523425c9b6056019c0c624d03785b55aa968154e",
    "pending":false,
    "amount":33913000,
    "fee":0,
    "memo":"€11.00 to Test",
    "time":1700823047,
    "bolt11":"lnbc339130n1pjkpqq8dq6u2p2cvf39ccrqgr5dus9getnwsxqz4usp5gvtfqs0e7ektphdc909jwv2kyyf3py5tfx9shtlm6338fdyxkrmspp5jw4zyxzwm8sv2a7u3dn4ydp9exmq2cqecrrzf5phsk6442tgz48qgv3atsymxs3chq24c9wqgwl3vh0cc0l3uvztsr365ft6lq35dg74ycazd9f77rc48aeywsswhl5x4e4zquxhelmen0yqt2zlps80u3gqj6ztam",
    "preimage":"0000000000000000000000000000000000000000000000000000000000000000",
    "payment_hash":"93aa22184ed9e0c577dc8b67523425c9b6056019c0c624d03785b55aa968154e",
    "expiry":1700823747.0,
    "extra":{
       "tag":"tpos",
       "tipAmount":1615,
       "tposId":"4qJSDrSf9peoV9iMqvmpBT",
       "amount":32298,
       "details":"{\"currency\":\"EUR\",\"exchangeRate\":2936.091232785993,\"items\":[{\"image\":null,\"price\":3,\"title\":\"Test Item 1\",\"description\":null,\"tax\":null,\"disabled\":false,\"formattedPrice\":\"€3.00\",\"id\":24,\"quantity\":2},{\"image\":\"\",\"price\":5,\"title\":\"New Beer\",\"description\":null,\"tax\":null,\"disabled\":false,\"formattedPrice\":\"€5.00\",\"id\":25,\"quantity\":1}]}",
       "wallet_fiat_currency":"EUR",
       "wallet_fiat_amount":11.745,
       "wallet_fiat_rate":2887.5556406306177
    },
    "wallet_id":"9fdb75d4e4ab480d88b1cd1defd35903",
    "webhook":"None",
    "webhook_status":"None"
 }
    )";

  DynamicJsonDocument doc(4096);
  // load into json doc and capture error
  DeserializationError error = deserializeJson(doc, paymentJson);
  if (error)
  {
    Serial.print(F("Test receipt deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }

  lastPayment = doc.as<JsonVariant>();
  hasReceiptToPrint = true;
  printReceipt();
  // clear lastPayment
  lastPayment = JsonVariant();
}

#endif