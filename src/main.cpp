#include <Arduino.h>
// ======================
// FOLDER
// ======================
#include "config.h"
#include "pins.h"
#include "sensors.h"
#include "global.h"
#include "hardware.h"
#include "control.h"
#include "weather.h"
#include "telegram.h"
#include "wifi_manager.h"
#include "time_utils.h"
#include "lcd_ui.h"
#include "telnet.h"
#include "web_server.h"
#include "mqtt_manager.h"



// ======================
// LIBRARY
// ======================
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include "esp_sleep.h"
#include <Wire.h>
#include <LittleFS.h>
#include <PubSubClient.h>

#define BLYNK_PRINT Serial
#include <BlynkSimpleEsp32.h> 
#include <Blynk/BlynkTimer.h>

// ======================
// FUNCTION DECLARATION
// ======================
void readSensor();
void systemControl();
void fetchWeather();

bool allowSmartWatering(int h);
bool isForecastRainAtHour(int h);

void startWater(unsigned long duration);

String weatherCode(String desc);

void lcdTask();
void lcdAuto();
void lcdManual();

void checkWiFi();
void setupOTA();

void lcdPrint16(String text);
void pumpStatusUpdate(bool state);
String buildStatusMessage();
void sendWateringReport( int h, int dur);
int progressPercent();
int dailyProgressPercent();

void weatherScheduler();
void telegramScheduler();
void nightManager();
void morningSync();
void checkResetButton();
void rainAlertTask();
void preWaterNotification();
void telnetTask();
void handleMQTTIP();



void disconnectBlynk(){
  Blynk.disconnect();
}

// ======================
// SETUP
// ======================
void setup(){
  Serial.begin(115200);
  delay(1000);

  LittleFS.begin(true);
  
  
  // ================= SAFE MODE =================
  WiFi.mode(WIFI_OFF);

  #ifdef SAFE_UPLOAD
  WiFi.mode(WIFI_OFF);
  Serial.println("SAFE UPLOAD MODE");
  return;
  #endif

  // ================= HARDWARE INIT =================
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // pastikan pompa OFF (aktif LOW)

  pinMode(RAIN_PIN, INPUT_PULLUP);
  pinMode(RESET_BTN_PIN, INPUT_PULLUP);

  // ================= LCD =================
  initHardware();
  lcd.init();
  lcd.backlight();

  // ================= I2C + ADS1115 =================
  Wire.begin(21, 22);
  Wire.setClock(100000);
  Wire.setTimeOut(50);
  if(!ads.begin()){
    Serial.println("ADS1115 gagal terdeteksi!");
  }

  // ================= SENSOR =================
  dht.begin();
  ds.begin();

  // ================= WIFI =================
    WiFi.mode(WIFI_STA);

    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);

    WiFi.begin(); // pakai SSID terakhir yang tersimpan

    unsigned long wifiStart = millis();

    while(WiFi.status() != WL_CONNECTED && millis() - wifiStart < 10000){

      delay(500);

      Serial.print(".");
      
      lcd.clear();
      lcd.setCursor(0,0);
      lcdPrint16("CONNECT WIFI");

      lcd.setCursor(0,1);
      lcdPrint16(String((millis()-wifiStart)/1000) + " DETIK");
    }

  
    if(WiFi.status() == WL_CONNECTED){

      wifiConnected = true;

      lcd.clear();
      lcd.setCursor(0,0);
      lcdPrint16("WIFI CONNECTED");

      lcd.setCursor(0,1);
      lcdPrint16(WiFi.localIP().toString());

      Blynk.config(BLYNK_AUTH_TOKEN);
      Blynk.connect(1000);

      //sendTelegram("ESP32 ONLINE\nhttp://" + WiFi.localIP().toString());
    }
    else{

      wifiConnected = false;

      lcd.clear();
      lcd.setCursor(0,0);
      lcdPrint16("OFFLINE MODE");

      lcd.setCursor(0,1);
      lcdPrint16("SYSTEM READY");

      // tetap lanjut boot!
    }

  // ================= MDNS =================
    Serial.println("\nWiFi connected");

    if (!MDNS.begin("esp32")) {
      Serial.println("Error starting mDNS");
      return;
    }

    Serial.println("mDNS started: http://esp32.local");

  // ================= OTA =================
  setupOTA();

  // ================= TELNET =================
  telnetServer.begin();
  telnetServer.setNoDelay(true);

  // ================= TIME =================
  configTime(8 * 3600, 0, "pool.ntp.org");

  int retry = 0;
  unsigned long ntpStart = millis();

  while(dayNow() == -1){

    ArduinoOTA.handle();
    yield();
    delay(10);

    if(millis() - ntpStart > 5000){
      Serial.println("NTP timeout");
      break;
    }
  }

  lastDay = dayNow();
  Serial.println("Time synced");

  setupTask();

  timer.setTimeout(15000, [](){

  sendTelegram(
    "ESP32 ONLINE\nhttp://" + WiFi.localIP().toString()
  );

});

  Serial.println("System Ready ✅");

  if(TEST_MODE){
    delay(3000); // tunggu WiFi stabil

  }

  // ================= WEB SERVER =================
    initWebServer();
    cleanupSessions();

  // ================= MQTT =================
    initMQTT();
    
  }




// ======================
// MAIN LOOP
// ======================
void loop(){

  ArduinoOTA.handle();

  runMQTT();

  static unsigned long lastMQTT = 0;

  if(millis() - lastMQTT > 2000){

      publishSensorData();

      lastMQTT = millis();
  }

  server.handleClient();

  wifiReconnectTask();

  if(portalMode) {
    delay(10);
    return;
  }

  if(WiFi.status() == WL_CONNECTED){
    Blynk.run();
  }

  static unsigned long lastLCD = 0;
  if(millis() - lastLCD > 300){
    lcdTask();     // ← lcdTask boleh pakai isRaining
    lastLCD = millis();
  }

  if(!otaActive){
    if(!nightMode){
    }
    timer.run();
  }

  yield(); // 🔥 PENTING: kasih waktu RTOS
  delay(1);

}  