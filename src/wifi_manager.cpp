#include "config.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>
#include <WebServer.h>
#include "wifi_manager.h"
#include "global.h"
#include "telegram.h"
#include "time_utils.h"
#include "pins.h"

void checkWiFi(){

  if(portalMode) return;

  if(otaActive || nightMode || portalMode) return;

  static unsigned long lastTry = 0;
  static unsigned long lostStart = 0;
  static bool sent = false;   // 🔥 pindahkan ke sini
  static bool mdnsStarted = false;  // 🔥 TAMBAHAN

  if(WiFi.status() != WL_CONNECTED){

    sent = false;  // 🔥 reset saat putus

    if(lostStart == 0){
      lostStart = millis();
    }

    if(millis() - lastTry > 10000){
      WiFi.reconnect();
      lastTry = millis();
    }

    if(millis() - lostStart > 30000){
      Serial.println("FORCE RECONNECT");

      WiFi.disconnect(true);
      delay(1000);
      WiFi.begin();

      lostStart = millis();
    }

  } else {

    lostStart = 0;

    String currentIP = WiFi.localIP().toString();
    int today = dayNow();

    // ===== LOGIKA ANTI SPAM =====
    bool ipChanged = (currentIP != lastSentIP);
    bool newDay = (today != lastSentDay);

    if(ipChanged || newDay){
      sendTelegram("✅ WiFi Connected\nhttp://" + currentIP);

      lastSentIP = currentIP;
      lastSentDay = today;
    }
  }
}

void openWiFiPortal() {

  Serial.println(">>> WIFI PORTAL MODE START");

  portalMode = true;

  // ===== STOP SEMUA SISTEM =====
  stopAllSystem();
  ArduinoOTA.end();
  server.stop();

  // ===== CLEAN WIFI STACK (INI KUNCI) =====
  WiFi.persistent(true);
  WiFi.disconnect(true, true);
  delay(1000);

  WiFi.mode(WIFI_OFF);
  delay(500);

  WiFi.mode(WIFI_STA);
  delay(1000);

  WiFi.setSleep(false);

  // ===== WIFI MANAGER =====
  WiFiManager wm;
  wm.setConfigPortalTimeout(180);

  bool res = wm.startConfigPortal(PORTAL_SSID);

  Serial.println(res ? "PORTAL OK" : "PORTAL FAIL");

  ESP.restart();
}

void wifiReconnectTask(){

    static bool lastWifiState = false;

    bool nowWifi = (WiFi.status() == WL_CONNECTED);

    // =========================
    // WIFI BARU CONNECT
    // =========================
    if(nowWifi && !lastWifiState){

        Serial.println("WiFi reconnect");

        sendPendingReports();
    }

    lastWifiState = nowWifi;
}

  // ======================
  // OTA
  // ======================
  void setupOTA(){
    ArduinoOTA.setHostname("ESP32-Watering");
    // ArduinoOTA.setPassword("agus08");

    ArduinoOTA.onStart([](){
      otaActive = true;
      pumpState = false;
      waterState = IDLE;          // 🔥 tambahan biar aman
      digitalWrite(RELAY_PIN, HIGH);


      lcd.backlight();
      lcd.clear();
      lcd.print("OTA MODE");
    });

    ArduinoOTA.onProgress([](unsigned int p,unsigned int t){
      int bar = (p*10)/t;
      lcd.setCursor(0,1);
      for(int i=0;i<10;i++) lcd.print(i<bar?"-":" ");
    });

    ArduinoOTA.onEnd([](){
      lcd.clear();
      lcd.print("OTA DONE");
      delay(1000);
      ESP.restart();
    });

    ArduinoOTA.begin();
  }