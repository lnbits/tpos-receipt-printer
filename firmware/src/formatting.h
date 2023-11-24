#ifndef FORMATTING_H
#define FORMATTING_H

#include <Arduino.h>

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

/**
 * @brief Add commas to a number
 * 
 */
String formatNumber(int number) {
    String formattedNumber = "";
    String numberString = String(number);
    int numberLength = numberString.length();
    int commaCount = 0;
    
    for (int i = numberLength - 1; i >= 0; i--) {
        formattedNumber = numberString[i] + formattedNumber;
        commaCount++;
        if (commaCount == 3 && i != 0) {
        formattedNumber = "," + formattedNumber;
        commaCount = 0;
        }
    }
    
    return formattedNumber;
}
#endif