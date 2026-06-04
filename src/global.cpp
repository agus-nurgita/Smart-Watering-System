#include "global.h"
#include "telegram.h"

// ================= SENSOR DATA =================
float tAir = 0;
float hAir = 0;

float tSoil = 0;
float tSoilRaw = 0;
float soilFiltered = 0;

int soilRaw = 0;
int soilPercent = 0;

int rainRaw = 0;
bool rainDetectedRaw = false;
bool rainStableState = false;
unsigned long rainChangeTimer = 0;

bool isRaining = false;

// ================= STATE =================
WaterState waterState = IDLE;

// ================= PUMP =================
bool pumpState = false;
bool pumpManual = false;

unsigned long pumpStartAuto = 0;
unsigned long pumpStartManual = 0;
unsigned long currentWaterDuration = 0;

const unsigned long MAX_PUMP_ON = 240000UL;
// ================= MODE =================
bool manualMode = false;
bool lastManualMode = false;

int seasonMode = 0;
// 0 = NORMAL
// 1 = KEMARAU
// 2 = HUJAN
int lastSeasonMode = 0;
SeasonMode currentSeason = NORMAL;

const char* getModeMusim(){
  if(seasonMode == 0) return "NORMAL";
  if(seasonMode == 1) return "KEMARAU";
  if(seasonMode == 2) return "HUJAN";
  return "-";
}

bool TEST_MODE = false;
bool justWokeUp = false;

// ================= SYSTEM =================
bool otaActive = false;
bool nightMode = false;
bool wifiConnected = false;

// ================= TIME =================
int lastDay = -1;
int lastSendHour = -1;

String formatHour(int h){
  if(h < 0) h = 0;
  if(h > 23) h = 23;
  char buf[6];
  sprintf(buf,"%02d:00", h);
  return String(buf);
}

// ================= BLYNK TIMER  =================
BlynkTimer timer;

// ================= WEB SERVER =================
WebServer server(80);

String historyURL = "";

// ================= AUTH =================
String adminUsername = "agus";
String adminPassword = "agus123";

String sessionTokens[MAX_SESSIONS];

unsigned long sessionTimes[MAX_SESSIONS];

const unsigned long SESSION_TIMEOUT = 1800000UL;

int loginFailCount = 0;

unsigned long loginBlockedUntil = 0;
// ================= LCD =================
void lcdPrint16(String text){
  if(text.length() > 16) text = text.substring(0,16);
  lcd.print(text);
}

// ================= WATER TRACKING =================
int dailyWaterDone = 0;
int lastWaterHourLogged = -1;

unsigned long WATER_DURATION = 30000UL;
unsigned long lastWaterTime = 0;
int lastWaterMinute = -1;
bool soilTooWet = false;


int waterHours[4] = {6, 10, 14, 17};
int waterReport[4] = {-1, -1, -1, -1};
bool waterSlotDone[4] = {false, false, false, false};

unsigned long lockedDuration[4] = {0,0,0,0};
String lockedDecision[4] = {"","","",""};

String wateringDecision = "-";

// ================= SCHEDULE =================
bool schedule6 = true;
bool schedule10 = true;
bool schedule14 = true;
bool schedule17 = true;

unsigned long decisionStartTime[4] = {0,0,0,0};
int userSelectedDuration[4] = {0,0,0,0};

// ================= WEATHER =================
String weatherP = "-";
String weatherE = "-";
String weatherS = "-";
String weatherN = "-";
bool weatherTaken[4] = {false,false,false,false};
int weatherHour[4] = {6,10,14,17};

// ================= RAIN =================
unsigned long rainStartDetect = 0;
unsigned long rainStopDetect  = 0;
unsigned long lastAlert       = 0;
bool lastRainState = false;

int rainPercent = 0;

// ================= TELEGRAM =================
long lastUpdateId = 0;
void sendTelegram(String message, String chatID);
String lastSentIP = "";
int lastSentDay = -1;
bool preNotifSent[4]={false,false,false,false};

int lastLoggedHour = -1;

bool pendingWaterReport[4] = {false, false, false, false};
int pendingDuration[4] = {0,0,0,0};

// ================= TELNET =================
WiFiServer telnetServer(23);
WiFiClient telnetClient;

// ================= UI =================
String animOldText = "";
String animNewText = "";
String animType = "";

bool animActive = false;
int animStep = 0;
unsigned long animLastUpdate = 0;

// ================= BUTTON =================
bool resetBtnHolding = false;
bool portalMode = false;

// ================= WATCHDOG =================
unsigned long lastLCDHeartbeat = 0;
unsigned long lastSensorHeartbeat = 0;
unsigned long lastControlHeartbeat = 0;

bool faultActive = false;

FaultType currentFault = FAULT_NONE;

bool waitingApproval = false;
bool recoveryRunning = false;