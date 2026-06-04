#ifndef GLOBAL_H
#define GLOBAL_H

#include "config.h" 
#include "hardware.h"
#include <Arduino.h>   // 🔥 WAJIB untuk String, millis, dll
#include <WebServer.h>
#include <Blynk/BlynkTimer.h>

// ================= SENSOR DATA =================
extern float tAir;
extern float hAir;

extern float tSoil;
extern float tSoilRaw;
extern float soilFiltered;

extern int soilRaw;
extern int soilPercent;

extern int rainRaw;
extern bool rainDetectedRaw;
extern bool rainStableState;
extern unsigned long rainChangeTimer;

extern bool isRaining;
// ================= STATE =================
enum WaterState { IDLE, WATERING };
extern WaterState waterState;


// ================= PUMP =================
extern bool pumpState;
extern bool pumpManual;

extern unsigned long pumpStartAuto;
extern unsigned long pumpStartManual;
extern unsigned long currentWaterDuration;

extern const unsigned long MAX_PUMP_ON;

// ================= MODE =================
extern bool manualMode;
extern bool lastManualMode;

extern int seasonMode;
extern int lastSeasonMode;

extern bool TEST_MODE;
extern bool justWokeUp;

extern const char* getModeMusim();

enum SeasonMode {NORMAL, KEMARAU, HUJAN};
extern SeasonMode currentSeason;

// ================= SYSTEM =================
extern bool otaActive;
extern bool nightMode;
extern bool wifiConnected;

// ================= TIME =================
int hourNow();
int minNow();
int dayNow();

extern int lastDay;
extern String formatHour(int h);
extern int lastSendHour;

// ================= WEB =================
extern WebServer server;

extern String historyURL;

// ================= AUTH =================
extern String adminUsername;
extern String adminPassword;

extern String sessionTokens[MAX_SESSIONS];

extern unsigned long sessionTimes[MAX_SESSIONS];

extern const unsigned long SESSION_TIMEOUT;

extern int loginFailCount;
extern unsigned long loginBlockedUntil;

// ================= WATER TRACKING =================
extern int dailyWaterDone;
extern int lastWaterHourLogged;

extern unsigned long WATER_DURATION;
extern unsigned long lastWaterTime;
extern int lastWaterMinute;
extern bool soilTooWet;

extern int waterHours[4];
extern int waterReport[4];
extern bool waterSlotDone[4];

extern unsigned long lockedDuration[4];
extern String lockedDecision[4];

extern String wateringDecision;

// ================= SCHEDULE =================
extern bool schedule6;
extern bool schedule10;
extern bool schedule14;
extern bool schedule17;

extern unsigned long decisionStartTime[4];
extern int userSelectedDuration[4];

// ================= WEATHER =================
extern String weatherP;
extern String weatherE;
extern String weatherS;
extern String weatherN;
extern bool weatherTaken[4];
extern int weatherHour[4];

// ================= RAIN =================
extern unsigned long rainStartDetect;
extern unsigned long rainStopDetect;
extern unsigned long lastAlert;
extern bool lastRainState;

extern int rainPercent;

// ================= TELEGRAM =================
extern long lastUpdateId;
extern String lastSentIP;
extern int lastSentDay;
extern bool preNotifSent[4];

extern int lastLoggedHour;

extern bool pendingWaterReport[4];
extern int pendingDuration[4];

// ================= TELNET =================
extern WiFiServer telnetServer;
extern WiFiClient telnetClient;

// ================= BLYNK =================
extern BlynkTimer timer;
void disconnectBlynk();


// ================= UI ANIMATION =================
extern String animOldText;
extern String animNewText;
extern String animType;
extern unsigned long animLastUpdate;

extern bool animActive;
extern int animStep;

// ================= BUTTON =================
extern bool resetBtnHolding;
extern bool portalMode;

// ================= WATCHDOG =================
extern unsigned long lastLCDHeartbeat;
extern unsigned long lastSensorHeartbeat;
extern unsigned long lastControlHeartbeat;


enum FaultType{

  FAULT_NONE,

  FAULT_LCD,
  FAULT_SENSOR,
  FAULT_CONTROL,
  FAULT_WIFI

};

extern bool faultActive;
extern FaultType currentFault;

extern bool waitingApproval;

extern bool recoveryRunning;
// ================= FUNCTION =================
void pumpStatusUpdate(bool state);
void sendWateringReport(int index, int hour, int duration);
void sendTelegram(String message, String chatID = TELEGRAM_CHAT_ID);
void lcdPrint16(String text);

#endif