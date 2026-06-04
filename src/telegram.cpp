#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "config.h"
#include "global.h"
#include "telegram.h"
#include "time_utils.h"

#include "sensors.h"
#include "control.h"
#include "weather.h"

void checkTelegramMessage() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.setTimeout(2000);

  String url = "https://api.telegram.org/bot" + String(TELEGRAM_TOKEN) +
               "/getUpdates?offset=" + String(lastUpdateId + 1);

  http.begin(url);
  int code = http.GET();

  if (code == 200) {

    String payload = http.getString();

    DynamicJsonDocument doc(6000);
    DeserializationError err = deserializeJson(doc, payload);

    if(err){
      Serial.println("JSON ERROR");
      http.end();
      return;
    }

    for (JsonObject result : doc["result"].as<JsonArray>()) {

      lastUpdateId = result["update_id"];

      // =========================
    // CALLBACK BUTTON
    // =========================
      if(result.containsKey("callback_query")){

          JsonObject cb = result["callback_query"];

          String data = cb["data"].as<String>();

          if(data == "FIX_FAULT"){

            // ===== ANTI DOUBLE CLICK =====
            if(recoveryRunning) return;

            recoveryRunning = true;

            waitingApproval = false;

            executeRecovery();
        }
      }

      // =========================
      // 💬 HANDLE MESSAGE BIASA
      // =========================
      if(result.containsKey("message")){

        String chatID = result["message"]["chat"]["id"].as<String>();
        String text   = result["message"]["text"].as<String>();

        text.trim();
        text.toLowerCase();

        if (text.indexOf("/start") >= 0) {
          sendTelegram("Halo! Tes bot Telegram berhasil 👍", chatID);
        }
        else if (text.indexOf("tes") >= 0) {
          sendTelegram(buildStatusMessage(), chatID);
        }
        else if (text.indexOf("ip") >= 0) {
          String ip = WiFi.localIP().toString();
          sendTelegram("🌐 LINK DASHBOARD:\nhttp://" + ip, chatID);
        }
      }
    }
  }
  else{
    Serial.println("HTTP ERROR: " + String(code));
  }

  http.end();
}

void morningSync(){
  if(hourNow() == 5){
    fetchWeather();
  }
}

void telegramScheduler(){
  Serial.println("Telegram check | nightMode=" + String(nightMode));
  
  if(nightMode) return; 
  checkTelegramMessage();
}

void sendTelegram(String message, String chatID) {

  if(nightMode) return;

  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;

  http.setTimeout(5000);   // ⬅️ DI SINI

  String url = "https://api.telegram.org/bot" + String(TELEGRAM_TOKEN) + "/sendMessage";

  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  DynamicJsonDocument doc(2048);
  doc["chat_id"] = chatID;
  doc["text"] = message;

  String payload;
  serializeJson(doc, payload);

  int code = http.POST(payload);

  if (code == 200) {
    Serial.println("Telegram terkirim!");
  } else {
    Serial.println("Gagal kirim Telegram: " + String(code));
  }

  http.end();
}

String buildStatusMessage() {

  // ===== Format Uptime =====
  unsigned long seconds = millis() / 1000;
  int hours = seconds / 3600;
  int minutes = (seconds % 3600) / 60;
  int secs = seconds % 60;

  String uptime = String(hours) + "j " + String(minutes) + "m " + String(secs) + "d";

  String modeMusim;
  if(seasonMode == 0) modeMusim = "NORMAL";
  else if(seasonMode == 1) modeMusim = "KEMARAU";
  else if(seasonMode == 2) modeMusim = "HUJAN";

  // ===== Kondisi Tanah =====
  String soilCondition;

  if (soilPercent < 60)
    soilCondition = "KERING";
  else if (soilPercent <75 )
    soilCondition = "LEMBAB";
  else 
    soilCondition = "BASAH";

  // ===== Terjemahan Keputusan Sistem =====
  String decisionText = wateringDecision;

  if (wateringDecision == "FULL")
    decisionText = "💧 FULL WATERING";

  else if (wateringDecision == "HALF")
    decisionText = "💧 HALF WATERING";

  else if (wateringDecision == "FORECAST_HALF")
    decisionText = "🌧 Forecast hujan → siram setengah";

  else if (wateringDecision == "BLOCK_SOIL")
    decisionText = "🌱 Tanah masih basah";

  else if (wateringDecision == "RAIN_STOP")
    decisionText = "☔ Hujan terdeteksi → penyiraman dihentikan";


  // ===== Pesan Telegram =====
  String msg = "🌿 SMART IRRIGATION STATUS\n";
  msg += "----------------------\n";
  msg += "🌦 Mode Musim : " + modeMusim + "\n";

  msg += "\n🌡 Suhu Udara : " + String(tAir) + " °C\n";
  msg += "💧 RH Udara   : " + String(hAir) + " %\n";

  msg += "\n🌱 DATA TANAH\n";
  msg += "🌡 Suhu Tanah : " + String(tSoil,1) + " °C\n";
  msg += "📟 ADC RAW    : " + String(soilRaw) + "\n";
  msg += "📊 ADC AVG    : " + String(soilFiltered) + "\n";
  msg += "💧 Soil       : " + String(soilPercent) + " %\n";
  msg += "🌱 Kondisi    : " + soilCondition + "\n";

  msg += "\n☔ Sensor Hujan : ";
  msg += (isRaining ? "HUJAN\n" : "TIDAK\n");

  msg += "🔮 Forecast     : ";
  msg += (isForecastRainAtHour(hourNow()) ? "HUJAN TERDETEKSI\n" : "AMAN\n");

  msg += "\n⚙ Mode  : ";
  msg += (nightMode ? "MALAM\n" : "SIANG\n");

  msg += "🚰 Pompa : ";
  msg += (pumpState ? "ON\n" : "OFF\n");

  msg += "\n💦 Keputusan\n";
  msg += decisionText + "\n";

  msg += "\n⏱ Uptime : " + uptime + "\n";

  msg += "----------------------";

  return msg;
}

void preWaterNotification() {

  int h = hourNow();
  int m = minNow();

  for (int i = 0; i < 4; i++) {

    int slotHour = waterHours[i];

    int notifHour = slotHour;
    int notifMinute = 55;

    notifHour = slotHour;
    notifMinute = 55;

    if(slotHour == 0) notifHour = 23;
    else notifHour = slotHour - 1;

    if (h == notifHour && m >= notifMinute && m < notifMinute + 2 && !preNotifSent[i]) {

      readSensor();  // ⬅️ WAJIB
      int getEstimatedDuration(int h);
      int duration = getEstimatedDuration(slotHour);
      bool forecastRain = isForecastRainAtHour(slotHour);

      bool rainNow = isRaining;

      if(rainNow){
        duration = 0;
      }

      String decisionText;
      String decisionPretty;

      if (rainNow) {
        decisionText = "☔ Sedang hujan\nPenyiraman akan dibatalkan";
        decisionPretty = "⛔ SKIP (HUJAN)";
      }
      else if (duration == 0) {
      decisionText = evaluateSmartDecisionReasonPretty(slotHour);
      decisionPretty = "❌ SKIP WATERING";
      }
      else if (forecastRain) {
        decisionText = "⚠️ Ramalan hujan terdeteksi\nPenyiraman akan setengah durasi";
        decisionPretty = "💧 HALF WATERING";
      }
      else {
        decisionText = "✅ Penyiraman AKAN DILAKSANAKAN";
        decisionPretty = "💧 FULL WATERING";
      }

      String message = "";

      message += "📢 INFO 5 MENIT SEBELUM SIRAM\n\n";
      message += "🌦 Mode Musim : ";
      message += getModeMusim();
      message += "\n";
      message += "\n⏰ Penyiraman jam ";
      message += formatHour(slotHour);
      message += "\n";
      message += decisionText;
      message += "\n\n";

      message += "⏱ Durasi siram : ";
      if (duration == 0) message += "skip";
      else message += String(duration) + " detik";

      message += "\n\n";

      message += "🌿 SMART IRRIGATION STATUS\n";
      message += "----------------------\n";

      message += "🌡 Suhu Udara : ";
      message += String(tAir);
      message += " °C\n";

      message += "💧 RH Udara   : ";
      message += String(hAir);
      message += " %\n\n";

      message += "🌱 DATA TANAH\n";

      message += "🔢 ADC : ";
      message += String(soilFiltered);
      message += "\n";

      message += "💧 Soil : ";
      message += String(soilPercent);
      message += " %\n\n";

      message += "☔ Sensor Hujan : ";
      message += isRaining ? "HUJAN" : "TIDAK";
      message += "\n";

      message += "🔮 Forecast : ";
      message += forecastRain ? "HUJAN" : "AMAN";
      message += "\n\n";

      message += "💦 Keputusan\n";
      message += decisionPretty;
      message += "\n";

      message += "----------------------\n\n";

      message += buildDailyWaterReport();

      sendTelegram(message);
      decisionStartTime[i] = millis();
      userSelectedDuration[i] = -1;

      preNotifSent[i] = true;
    }

    if (h != notifHour) {
      preNotifSent[i] = false;
    }
  }
}

void rainAlertTask() {

  bool currentState = isRaining;

  Serial.println("RAIN ALERT CHECK: " + String(currentState));

  // ===== HUJAN MULAI =====
  if(currentState && !lastRainState){

    Serial.println("HUJAN TERDETEKSI → KIRIM TELEGRAM");

    if(!nightMode){
      sendTelegram("⚠️ PERINGATAN HUJAN\nSensor mendeteksi HUJAN 🌧️");
      lastAlert = millis();
    }

    lastRainState = true;
  }

  // ===== HUJAN BERHENTI =====
  if(!currentState && lastRainState){

    Serial.println("HUJAN BERHENTI");

    if(!nightMode){
      sendTelegram("ℹ️ INFO CUACA\nHujan telah berhenti ☀️");
      lastAlert = millis();
    }

    lastRainState = false;
  }
}

int getWateringIndex(int hourSlot){

    for(int i=0; i<4; i++){

        if(waterHours[i] == hourSlot){
            return i;
        }
    }

    return 0;
}

void sendWateringReport(int hourSlot, int duration){

  int index = getWateringIndex(hourSlot);

  String msg = "🌿 LAPORAN PENYIRAMAN\n\n";

  msg += "⏰ Jadwal : ";
  msg += formatHour(hourSlot);
  msg += "\n";

  if(duration == 0){
    msg += "⛔ Penyiraman DIBATALKAN\n";
  } 
  else{
    msg += "✅ Penyiraman SELESAI\n";
  }

  msg += "💧 Durasi : ";
  msg += String(duration);
  msg += " detik\n";

  msg += "🌱 Soil : ";
  msg += String(soilPercent);
  msg += "% (";
  msg += soilCondition();
  msg += ")\n";

  msg += "☔ Status hujan : ";
  msg += (isRaining ? "Hujan" : "Tidak");

   // =========================
  // WIFI CHECK
  // =========================
  if(WiFi.status() == WL_CONNECTED){

      sendTelegram(msg);

    }
    else{

      pendingWaterReport[index] = true;
      pendingDuration[index] = duration;

      Serial.println("WiFi OFF → report disimpan");
  }
}

void sendPendingReports(){

    if(WiFi.status() != WL_CONNECTED) return;

    for(int i=0; i<4; i++){

        if(pendingWaterReport[i]){

            Serial.println("Mengirim pending report...");

            sendWateringReport(
                waterHours[i],
                pendingDuration[i]
            );

            pendingWaterReport[i] = false;

            delay(1000);
        }
    }
}


void sendRecoveryApproval(String msg){

  if(WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;

  String url =
    "https://api.telegram.org/bot" +
    String(TELEGRAM_TOKEN) +
    "/sendMessage";

  http.begin(url);

  http.addHeader("Content-Type", "application/json");

  String payload = "{";
  
  payload += "\"chat_id\":\"" + String(TELEGRAM_CHAT_ID) + "\",";
  
  payload += "\"text\":\"" + msg + "\",";
  
  payload += "\"reply_markup\":{";
  
  payload += "\"inline_keyboard\":[[";
  
  payload += "{";
  
  payload += "\"text\":\"KERJAKAN\",";
  
  payload += "\"callback_data\":\"FIX_FAULT\"";
  
  payload += "}";
  
  payload += "]]}";
  
  payload += "}";

  http.POST(payload);

  http.end();
}
