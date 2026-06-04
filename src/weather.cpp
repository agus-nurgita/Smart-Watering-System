#include <Arduino.h>
#include <WiFi.h>
#include "global.h"
#include "time_utils.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "config.h"
#include "weather.h"

void fetchWeather(){

  if(WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.setTimeout(5000);

  String url = "http://api.openweathermap.org/data/2.5/forecast?lat=" + String(LAT) +
               "&lon=" + String(LON) +
               "&appid=" + String(API_KEY) +
               "&units=metric";

  http.begin(url);
  int code = http.GET();

  if(code != 200){
    http.end();
    return;
  }

  String payload = http.getString();

  DynamicJsonDocument doc(5000);
  DeserializationError err = deserializeJson(doc, payload);

  if(err || !doc.containsKey("list")){
    http.end();
    return;
  }

  // ===== SLOT JAM SESUAI SISTEM =====
  int targetHours[4] = {6,10,14,17};
  String* weatherVars[4] = {&weatherP, &weatherE, &weatherS, &weatherN};

  JsonArray list = doc["list"];

  for(int i = 0; i < 4; i++){

    int targetHour = targetHours[i];
    int minDiff = 24;
    JsonObject bestMatch;

    for(JsonObject item : list){

      const char* dt = item["dt_txt"];
      if(!dt) continue;

      // ambil jam dari string "YYYY-MM-DD HH:MM:SS"
      int h = (dt[11]-'0')*10 + (dt[12]-'0');

      int diff = abs(h - targetHour);

      if(diff < minDiff){
        minDiff = diff;
        bestMatch = item;
      }
    }

    // ===== AMBIL WEATHER =====
    if(!bestMatch.isNull() && bestMatch["weather"].size() > 0){

      const char* mainWeather = bestMatch["weather"][0]["main"];
      *weatherVars[i] = weatherCode(String(mainWeather));

    } else {
      *weatherVars[i] = "?";
    }
  }

  http.end();
}

bool isForecastRainAtHour(int hour){

  if(hour == 6)  return (weatherP == "H" || weatherP == "F");
  if(hour == 10) return (weatherE == "H" || weatherE == "F");
  if(hour == 14) return (weatherS == "H" || weatherS == "F");
  if(hour == 17) return (weatherN == "H" || weatherN == "F");

  return false;
}

String weatherCode(String desc){

  desc.toLowerCase();

  if(desc.indexOf("rain")    >= 0) return "H";
  if(desc.indexOf("drizzle") >= 0) return "F";
  if(desc.indexOf("cloud")   >= 0) return "C";
  if(desc.indexOf("clear")   >= 0) return "-";

  return "?";
}

void weatherScheduler(){
  fetchWeather();
}