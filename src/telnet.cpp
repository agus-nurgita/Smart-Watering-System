#include "telnet.h"

#include "global.h"
#include "time_utils.h"
#include "telegram.h"
#include "control.h"
#include "sensors.h"

#include <WiFi.h>

// 🔥 extern object dari .ino
extern WiFiServer telnetServer;
extern WiFiClient telnetClient;

void telnetTask(){

  if(nightMode || otaActive) return;

  // ambil client baru
  if(!telnetClient || !telnetClient.connected()){
    telnetClient = telnetServer.available();
    return;
  }

  // ========================
  // OPTIONAL COMMAND
  // ========================
  if(telnetClient.available()){
    String cmd = telnetClient.readStringUntil('\n');
    cmd.trim();

    if(cmd.equalsIgnoreCase("tes")){
      sendTelegram(buildStatusMessage());
    }

    if(cmd.equalsIgnoreCase("ip")){
      telnetClient.println("STA IP: " + WiFi.localIP().toString());
      telnetClient.println("AP  IP: " + WiFi.softAPIP().toString());
    }
  }

  // ========================
  // MODE DETECTION
  // ========================
  String modeNet;
  String ipAddr;

  if(WiFi.getMode() == WIFI_AP_STA){
    modeNet = "HYBRID (AP+STA)";
    ipAddr = WiFi.softAPIP().toString();
  }
  else if(WiFi.getMode() == WIFI_AP){
    modeNet = "AP ONLY";
    ipAddr = WiFi.softAPIP().toString();
  }
  else{
    modeNet = "STA MODE";
    ipAddr = WiFi.localIP().toString();
  }

  // ========================
  // OUTPUT DASHBOARD
  // ========================
  telnetClient.println("\n===== ESP32 PLANT WATERING =====");
  telnetClient.println("NETWORK MODE: " + modeNet);

  telnetClient.print("TIME        : ");
  telnetClient.println(timeNow());

  telnetClient.println(String("MODE        : ") + (manualMode ? "MANUAL" : "AUTO"));
  telnetClient.println(String("PUMP        : ") + (pumpState ? "ON" : "OFF"));
  telnetClient.println(String("RAIN        : ") + (isRaining ? "HUJAN" : "TIDAK HUJAN"));

  telnetClient.println("--------------------------------");

  telnetClient.println("SOIL RAW    : " + String(soilFiltered));
  telnetClient.println("SOIL (%)    : " + String(soilPercent) + " %");
  telnetClient.println("TEMP AIR    : " + String(tAir,1) + " C");
  telnetClient.println("HUM AIR     : " + String(hAir,1) + " %");
  telnetClient.println("TEMP SOIL   : " + String(tSoil,1) + " C");

  telnetClient.println("--------------------------------");

  telnetClient.println(String("WATER STATE : ") + (waterState==WATERING ? "WATERING" : "IDLE"));
  telnetClient.println("PROGRESS    : " + String(progressPercent()) + " %");

  telnetClient.println("--------------------------------");

  telnetClient.println(String("NIGHT MODE  : ") + (nightMode ? "AKTIF" : "OFF"));
  telnetClient.println(String("OTA MODE    : ") + (otaActive ? "AKTIF" : "OFF"));
  telnetClient.println("WIFI RSSI   : " + String(WiFi.RSSI()) + " dBm");

  telnetClient.println("IP ADDRESS  : " + ipAddr);

  telnetClient.println("================================");

  telnetClient.println("RAIN RAW    : " + String(rainRaw));
  telnetClient.println("RAIN STATE  : " + String(isRaining ? "HUJAN" : "TIDAK"));

  telnetClient.println("DS18B20 RAW : " + String(tSoilRaw,2));
  telnetClient.println("DS18B20 OK  : " + String(tSoil,2));
}
