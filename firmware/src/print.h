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
extern const bool printToSerialOnly;

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

  if (!printToSerialOnly)
  {
    printer.wake();
    printer.print(receipt);
    printer.sleep();
  }
}

void printConnectedToWifi()
{
  String receipt = R"(
 Connected to $$ssid


    )";
  receipt.replace("$$ssid", ssid);
  Serial.print(receipt);

  if (!printToSerialOnly)
  {
    printer.wake();
    printer.print(receipt);
    printer.sleep();
  }
}

void printWelcomeReceipt()
{
  String receipt = R"(

 $$companyName
)";

  receipt.replace("$$companyName", companyName);
  Serial.print(receipt);
  if (!printToSerialOnly)
  {
    printer.wake();
    printer.print(receipt);
    printer.sleep();
  }
}

/**
 * @brief Get the Exchange Rate from the exchangeRate property in the extra.details JSON string
 *
 * @param paymentExtraDetails
 * @return float
 */
float getExchangeRate(String paymentExtraDetails)
{
  Serial.println("Getting exchange rate payment extra details: " + paymentExtraDetails);
  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, paymentExtraDetails);
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return 0;
  }
  float exchangeRate = 0;
  try
  {
    exchangeRate = doc["exchangeRate"].as<float>();
    // this is sats per currency unit, convert to currency per bitcoin (100,000,000 sats)
    float fiatExchangeRatePerBtc = 100000000 / exchangeRate;
    Serial.println("fiatExchangeRatePerBtc: " + String(fiatExchangeRatePerBtc));
    return fiatExchangeRatePerBtc;
  }
  catch (const std::exception &e)
  {
    Serial.println("Error getting exchangeRate from extra.details");
    Serial.println(e.what());
    return 0;
  }
}

/**
 * @brief Get the Currency object from the currency property in the extra.details JSON string
 *
 * @param paymentExtraDetails
 * @return String
 */
String getCurrency(String paymentExtraDetails)
{
  Serial.println("getting currency Payment extra details: " + paymentExtraDetails);
  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, paymentExtraDetails);
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return "";
  }
  String currency = "";
  try
  {
    currency = doc["currency"].as<String>();
  }
  catch (const std::exception &e)
  {
    Serial.println("Error getting currency from extra.details");
    Serial.println(e.what());
    return "";
  }
  return currency;
}

/**
 * @brief Get a string of payment items from the extra.details JSON string
 *
 * @param paymentExtraDetails
 */
String getPaymentItems(String paymentExtraDetails)
{
  Serial.println("getting payment items Payment extra details: " + paymentExtraDetails);
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
  // loop through items and add to itemsString using title, quantity and price
  for (JsonVariant item : items)
  {
    String title = item["title"].as<String>();
    int quantity = item["quantity"].as<int>();
    float price = item["price"].as<float>();
    itemsString += title + " x " + String(quantity) + "   " + String(price) + "\n";
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
    String fiatTotal = memo.substring(1, memo.indexOf(" "));

    int amountMSats = lastPayment["amount"].as<int>();

    String tag = lastPayment["extra"]["tag"].as<String>();
    int tipAmount = lastPayment["extra"]["tipAmount"].as<int>();
    String paymentHash = lastPayment["payment_hash"].as<String>();
    int fee = lastPayment["fee"].as<int>();
    float walletFiatAmount = 0;
    String walletFiatCurrency = "";

    String extraDetails = lastPayment["extra"]["details"];

    Serial.println("extraDetails: " + extraDetails);

    walletFiatCurrency = getCurrency(extraDetails);
    
    String fiatExchangeRate = "";
    float walletFiatRate = 0;
    walletFiatRate = getExchangeRate(extraDetails);
    fiatExchangeRate = formatNumber(walletFiatRate);

    // if we have a walletFiatRate (sats per currency unit), create a subTotalMSats fiat value
    String subTotalFiat = "";
    String tipsFiat = "";
    if (walletFiatRate > 0 && amountMSats > 0)
    {
      float subTotalFiatFloat = (static_cast<float>(amountMSats) / 1000 - tipAmount) / 100000000 * walletFiatRate;
      Serial.println("subTotalFiatFloat: " + String(subTotalFiatFloat));
      subTotalFiat = String(subTotalFiatFloat, 2);
      subTotalFiat += " " + walletFiatCurrency;
      // same for tips
      float tipsFiatFloat = tipAmount / walletFiatRate;
      tipsFiat = String(tipsFiatFloat, 2);
      tipsFiat += " " + walletFiatCurrency;
    }

    // if extras.details exists, get the itemsDetail
    String itemsDetail = "";
    Serial.println("Getting items from extra.details" + extraDetails);
    itemsDetail = "\r\n" + getPaymentItems(extraDetails);

    // search and replace in receipt
    receipt.replace("$$companyName", companyName);
    receipt.replace("$$time", time);
    receipt.replace("$$memo", memo);
    receipt.replace("$$items", itemsDetail);
    receipt.replace("$$amount", formatNumber(amountMSats / 1000 - tipAmount));
    receipt.replace("$$subTotalFiat", "- " + subTotalFiat);
    receipt.replace("$$tip ", formatNumber(tipAmount) + " ");
    receipt.replace("$$tipsFiat", "- " + tipsFiat);
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

    if (!printToSerialOnly)
    {
      printer.wake();
      printer.print(receipt);
      printer.sleep();
    }
  }
  hasReceiptToPrint = false;
}

void printTestReceipt()
{
  String paymentJson = R"(
  {
  "wallet_balance": 416511,
  "payment": {
    "checking_id": "59a1544d23846b8c2d6efaf7dbbc4fd216e54d5d7f8440d6829dcc5126828b98",
    "pending": false,
    "amount": 7951000,
    "fee": 0,
    "memo": "€3.10 to Cake and Coffee",
    "time": 1705581662,
    "bolt11": "lnbc79510n1pj6j8zusp5fjkgksphs5vf2sdhtr4eqyyskzmvdxz9en0v7jdmmde06j5uurlqpp5txs4gnfrs34ccttwltmah0z06gtw2n2a07zyp45znhx9zf5z3wvqdp2u2p2cvewxyczqar0yppkz6m9ypskuepqgdhkven9v5xqzjccqpjrzjqdva2ltch9a03q8jlmxye9awwsqxfkgjge4nr0hdj5lvg865yn7jkz7pqqqq9hsqqyqqqqqqqqqqqqgqyg9qxpqysgq9qj8qzm02lk8jemd9wtj745zpvxmkd3adlz5zrw8w4g4we4930uy6vrwgxgrm070xclf2d83g8fe4g68j00wzr9j52d44hk3haeg4mspjjstz4",
    "preimage": "0000000000000000000000000000000000000000000000000000000000000000",
    "payment_hash": "59a1544d23846b8c2d6efaf7dbbc4fd216e54d5d7f8440d6829dcc5126828b98",
    "expiry": 1705582260,
    "extra": {
      "tag": "tpos",
      "tipAmount": 0,
      "tposId": "FZVreZvcnnjbuqBX3EoKpY",
      "amount": false,
      "details": "{\"currency\":\"EUR\",\"exchangeRate\":2564.6076289048888,\"items\":[{\"price\":3.1,\"formattedPrice\":\"€3.10\",\"quantity\":1,\"title\":\"Coffee and Walnut cake\",\"tax\":10}]}"
    },
    "wallet_id": "845ad088a1604d96ba039b7d6efe87ab",
    "webhook": null,
    "webhook_status": null
    }
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