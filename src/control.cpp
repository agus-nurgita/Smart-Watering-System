#include <Arduino.h>
#include <WiFi.h>
#include "control.h"
#include "global.h"
#include "pins.h"
#include "sensors.h"
#include "weather.h"
#include "telegram.h"
#include "wifi_manager.h"
#include "time_utils.h"
#include "telnet.h"
#include "lcd_ui.h" 
#include <Web_Server.h> 

#include <Blynk/BlynkTimer.h>
#include <WebServer.h>



void startWater(unsigned long duration){
  waterState = WATERING;
  pumpStartAuto = millis();
  currentWaterDuration = duration;   // ← SIMPAN KE SINI
}

void pumpStatusUpdate(bool state){
}


void systemControl(){

  lastControlHeartbeat = millis();

if(TEST_MODE){
  // hanya jalankan state pompa saja
  if (waterState == WATERING) {

    pumpState = true;

    if (millis() - pumpStartAuto >= currentWaterDuration) {
      waterState = IDLE;
      pumpState = false;
    }

  } else {
    pumpState = false;
  }

  digitalWrite(RELAY_PIN, pumpState ? LOW : HIGH);
  return;
}

  // ===== GLOBAL STOP =====
  if(otaActive || nightMode){
    pumpState = false;
    waterState = IDLE;
    digitalWrite(RELAY_PIN, HIGH);
    return;
  }

  // ===== DETEKSI MODE MUSIM =====
  if(seasonMode != lastSeasonMode){
    animOldText = (lastSeasonMode == 0) ? "NORMAL" :
                  (lastSeasonMode == 1) ? "KEMARAU" : "HUJAN";

    animNewText = (seasonMode == 0) ? "NORMAL" :
                  (seasonMode == 1) ? "KEMARAU" : "HUJAN";

    animType = "SEASON";
    animActive = true;
    animStep = 0;

    lastSeasonMode = seasonMode;
  }

  // ===== DETEKSI AUTO / MANUAL =====
  if(manualMode != lastManualMode){
    animOldText = lastManualMode ? "MANUAL" : "AUTO";
    animNewText = manualMode ? "MANUAL" : "AUTO";

    animType = "MODE";
    animActive = true;
    animStep = 0;

    lastManualMode = manualMode;
  }
  // ======================================================
  // ===================== MANUAL MODE ====================
  // ======================================================
  if(manualMode){

    if(pumpManual && !pumpState){
    pumpStartManual = millis();
    }

    pumpState = pumpManual;

    // proteksi max durasi
    if(pumpState && millis() - pumpStartManual > MAX_PUMP_ON){
      pumpState = false;
      pumpManual = false;
    }

    digitalWrite(RELAY_PIN, pumpState ? LOW : HIGH);
    pumpStatusUpdate(pumpState);
    return;
  }

  // ======================================================
  // ====================== AUTO MODE =====================
  // ======================================================
  int h = hourNow();
  int m = minNow();

  // ===== RESET HARIAN =====
  int today = dayNow();
  if(today != -1 && today != lastDay){

    dailyWaterDone = 0;

    for(int i=0;i<4;i++){
      waterReport[i] = -1;
      waterSlotDone[i] = false;  // ⬅ reset slot flag juga

      lockedDuration[i] = 0;      // tambah
      lockedDecision[i] = "";     // tambah
    }

    lastDay = today;
    Serial.println("Reset harian berhasil");
  }

  // ======================================================
  // ===================== SCHEDULE HIT ===================
  // ======================================================
  int slotIndex = -1;
if(h == 6 && m == 0) slotIndex = 0;
else if(h == 10 && m == 0) slotIndex = 1;
else if(h == 14 && m == 0) slotIndex = 2;
else if(h == 17 && m == 0) slotIndex = 3;

static int lastExecutedHour = -1;

if(
  slotIndex >= 0 &&
  waterState == IDLE &&
  h != lastExecutedHour &&
  (
    (slotIndex==0 && schedule6) ||
    (slotIndex==1 && schedule10) ||
    (slotIndex==2 && schedule14) ||
    (slotIndex==3 && schedule17)
  )
){

    if(!waterSlotDone[slotIndex]){
    int dur = getAutoWaterDurationAI(h);

    if(seasonMode == 2 && isRaining){
      dur = 0;
    }

    if(dur == 0){
      waterReport[slotIndex] = 0;
      sendWateringReport(h, 0);
      waterSlotDone[slotIndex] = true;
    } 
    else{
      startWater(dur * 1000UL);
    }

    lastExecutedHour = h;
}

}
  // ======================================================
  // ================= WATERING PROCESS ===================
  // ======================================================
  if (waterState == WATERING) {

  pumpState = true;

  unsigned long elapsed = millis() - pumpStartAuto;

  if (elapsed >= currentWaterDuration || elapsed >= MAX_PUMP_ON) {

    int h = hourNow();

    // ===== RESET DATA DULU (PENTING) =====
    int dur = currentWaterDuration / 1000;

    currentWaterDuration = 0;
    pumpStartAuto = 0;

    waterState = IDLE;
    pumpState = false;

    // ===== LOG HARIAN =====
    if(h != lastWaterHourLogged){
      dailyWaterDone++;
      lastWaterHourLogged = h;

      for(int i=0;i<4;i++){
        if(h == waterHours[i]){

          waterReport[i] = dur;

          sendWateringReport(h, dur);
          
          waterSlotDone[i] = true;  // ✅ WAJIB
        }
      }
    }

    wateringDecision = "-";
  }

} 
else {
  pumpState = false;
}

digitalWrite(RELAY_PIN, pumpState ? LOW : HIGH);
pumpStatusUpdate(pumpState);
  // ======================================================
  // ================= OUTPUT CONTROL =====================
  // ======================================================
  digitalWrite(RELAY_PIN, pumpState ? LOW : HIGH);
  pumpStatusUpdate(pumpState);

  // ✅ RESET LOCK (WAJIB DI SINI)
    if(minNow() != 0){
      lastExecutedHour = -1;
}
}


int getEstimatedDuration(int h){

  if(isRaining){
    wateringDecision = "RAIN_STOP";
    return 0;
  }

  if(isForecastRainAtHour(h)){
    wateringDecision = "FORECAST_HALF";
    return 40; // MAX 40 DETIK
  }

  wateringDecision = "FULL";
  return WATER_DURATION / 1000;
}

// ======================
// SMART DECISION ENGINE
// ======================
int getAutoWaterDurationAI(int slotHour){

  // ===== PRIORITAS 1: HUJAN LANGSUNG STOP =====
  if(isRaining){
    wateringDecision = "RAIN_STOP";
    return 0;
  }

  // ===== PRIORITAS 2: FORECAST HUJAN =====
  bool forecastRain = isForecastRainAtHour(slotHour);

  // ===== HYSTERESIS DINAMIS =====
  int SOIL_WET_LOCK;
  int SOIL_WET_RELEASE;

  if(seasonMode == 0){ // NORMAL
    SOIL_WET_LOCK = 85;
    SOIL_WET_RELEASE = 80;
  }
  else if(seasonMode == 1){ // KEMARAU
    SOIL_WET_LOCK = 90;
    SOIL_WET_RELEASE = 85;
  }
  else if(seasonMode == 2){ // HUJAN
    SOIL_WET_LOCK = 75;
    SOIL_WET_RELEASE = 70;
  }

  // ===== HYSTERESIS =====
  if(soilPercent >= SOIL_WET_LOCK) soilTooWet = true;
  if(soilPercent <= SOIL_WET_RELEASE) soilTooWet = false;

  if(soilTooWet){

  if(tAir >= 34 && hAir <= 50){
      wateringDecision = "HALF";
      return 10;
    }

    wateringDecision = "BLOCK_SOIL";
    return 0;
  }

  // ===== BASE DURATION =====
  int dur = 50;

  if(soilPercent < 50) dur += 15;
  else if(soilPercent < 65) dur += 10;
  else if(soilPercent < 70) dur += 5;
  else if(soilPercent > 85) dur -= 10;

  // ===== MODE MUSIM =====
  if(seasonMode == 1) dur += 10;
  else if(seasonMode == 2) dur -= 5;

  // ===== SUHU =====
  if(tAir >= 34) dur += 10;
  else if(tAir >= 31) dur += 5;

  // ===== KELEMBABAN =====
  if(hAir <= 50) dur += 10;
  else if(hAir <= 60) dur += 5;

  // ===== FORECAST =====
  if(forecastRain){
    dur *= 0.5;
    wateringDecision = "FORECAST_HALF";
  } else {
    wateringDecision = "FULL";
  }

  // ===== LIMIT =====
  if(seasonMode == 0) dur = constrain(dur, 25, 90);
  else if(seasonMode == 1) dur = constrain(dur, 40, 120);
  else if(seasonMode == 2) dur = constrain(dur, 20, 40);


  if(dur <= 0){
    wateringDecision = "BLOCK_SOIL";
    return 0;
  }

  return dur;
}

// ======================
// WATER CONTROL
// ======================
unsigned long remainPhase(){
  if (waterState != WATERING) return 0;

  unsigned long elapsed = millis() - pumpStartAuto;
  if(elapsed >= currentWaterDuration) return 0;

  return (currentWaterDuration - elapsed) / 1000;
}

String getWaterStatus(int h, bool enabled){

  String status = enabled ? "N" : "F";

  for(int i=0;i<4;i++){
    if(h == waterHours[i]){

      if(waterReport[i] == -1) return status + "?";
      if(waterReport[i] == 0)  return status + "X";
      return status + "✓";
    }
  }
  return status + "?";
}

int dailyTargetCount(){
  int count = 0;
  if(schedule6) count++;
  if(schedule10) count++;
  if(schedule14) count++;
  if(schedule17) count++;
  return count;
}

int progressPercent(){
  if (waterState != WATERING) return 0;

  unsigned long elapsed = millis() - pumpStartAuto;
  if (elapsed >= currentWaterDuration) return 100;

  return (elapsed * 100) / currentWaterDuration;
}

int dailyProgressPercent(){
  int target = dailyTargetCount();
  if(target == 0) return 0;
  return (dailyWaterDone * 100) / target;
}


String evaluateSmartDecisionReasonPretty(int h){

  if(wateringDecision == "FULL")
    return "💧 FULL WATERING";

  if(wateringDecision == "HALF")
    return "💧 HALF WATERING";

  if(wateringDecision == "FORECAST_HALF")
    return "⚠️ Forecast hujan → siram setengah";

  if(wateringDecision == "BLOCK_SOIL")
    return "🌱 Tanah masih basah";

  if(wateringDecision == "RAIN_STOP")
    return "☔ Hujan terdeteksi → dihentikan";

  if(wateringDecision == "SYSTEM_OFF")
    return "🚫 Sistem dimatikan manual";

  return "❌ SKIP WATERING";
}

String soilCondition(){
  if(soilPercent < 60) return "KERING";
  else if(soilPercent < 75) return "LEMBAB";
  else if(soilPercent < 85) return "BASAH";
  return "SANGAT BASAH";  // 🔥 tambahan
}

// ======================
// BUTTON CONTROL
// ======================
void checkResetButton(){

  static bool btnActive = false;
  static unsigned long pressStart = 0;
  static bool actionDone = false;

  bool pressed = (digitalRead(RESET_BTN_PIN) == LOW);

  // ===== SAAT DITEKAN =====
  if(pressed){

    if(!btnActive){
      btnActive = true;
      pressStart = millis();
      actionDone = false;
    }

    resetBtnHolding = true;

    unsigned long holdTime = millis() - pressStart;

    int sec = holdTime / 1000;

    lcd.clear();
    lcd.setCursor(0,0);
    lcdPrint16("TAHAN: " + String(sec) + " DETIK");

    lcd.setCursor(0,1);

    if(holdTime >= 10000){
      lcdPrint16("RESTART...");
    }
    else if(holdTime >= 5000){
      lcdPrint16("PORTAL WIFI");
    }
    else if(holdTime >= 1000){
      lcdPrint16("RESET WIFI");
    }
    else{
      lcdPrint16("TUNGGU...");
    }
  }

  // ===== SAAT DILEPAS =====
  else {

    resetBtnHolding = false;

    if(btnActive && !actionDone){

      unsigned long holdTime = millis() - pressStart;

      // =========================
      // 10 DETIK → RESTART
      // =========================
      if(holdTime >= 10000){
        Serial.println("RESTART ESP32");
        ESP.restart();
      }

      // =========================
      // 5 DETIK → WIFI PORTAL
      // =========================
      else if(holdTime >= 5000){
        openWiFiPortal();
      }
      // =========================
      // 1 DETIK → DISCONNECT WIFI
      // =========================
      else if(holdTime >= 1000){
        Serial.println("DISCONNECT WIFI");

        WiFi.disconnect(true);
        delay(500);
        WiFi.reconnect();
      }

      actionDone = true;
      btnActive = false;
    }
  }
}

String buildDailyWaterReport(){

  String msg = "";
  int total = 0;

  msg += "🌿 Status Penyiraman Hari Ini\n";
  msg += "━━━━━━━━━━━━━━━━━━\n\n";

  for(int i=0;i<4;i++){

    msg += "🕒 ";
    msg += formatHour(waterHours[i]);
    msg += "  ";

    if(waterReport[i] == -1){

      msg += "⏳ Menunggu";

    }
    else if(waterReport[i] == 0){

      msg += "⛔ Skip";

    }
    else{

      msg += "💧 ";
      msg += String(waterReport[i]);
      msg += " detik";

      total += waterReport[i];
    }

    msg += "\n";
  }

  msg += "\n━━━━━━━━━━━━━━━━━━\n";
  msg += "💧 Total Air : ";
  msg += String(total);
  msg += " detik\n";

  return msg;
}

// ======================
// AUTO RESTAT SISTEM
// ======================
void autoRestartTask(){

  static int lastRestartDay = -1;

  int h = hourNow();
  int m = minNow();
  int d = dayNow();

  // setiap tanggal kelipatan 3 → restart
  if(h == 3 && m == 0){

    if(d != lastRestartDay){

      if(d % 3 == 0){   // 🔥 inti logika 3 hari
        Serial.println("AUTO RESTART (SETIAP 3 HARI)");
        delay(1000);
        ESP.restart();
      }

      lastRestartDay = d;
    }
  }
}

// ======================
// STOP ALL SYSTEM
// ======================
void stopAllSystem(){
  timer.disableAll();
  disconnectBlynk();
}

// ======================
// TASK SCHEDULER
// ======================
void setupTask(){
  timer.setInterval(1000L, systemControl);
  timer.setInterval(15000L, readSensor);
  timer.setInterval(20000L, checkWiFi);
  timer.setInterval(3000L, telnetTask);
  timer.setInterval(10800000L, weatherScheduler); // tiap 3 jam
  timer.setInterval(3000L, morningSync);
  timer.setInterval(2000L, nightManager);
  timer.setInterval(100L, checkResetButton);
  timer.setInterval(60000L, telegramScheduler); // 1 menit
  timer.setInterval(3000L, rainAlertTask);   // cek hujan tiap 3 detik
  timer.setInterval(20000L, preWaterNotification);
  timer.setInterval(60000L, logHourlyData);
  timer.setInterval(60000L, resetDailyLog);
  timer.setInterval(1000L, rainStabilityFilter);
  timer.setInterval(2000L, watchdogMonitor);
}

void watchdogMonitor(){

  unsigned long now = millis();

  // ================= LCD =================
  if(now - lastLCDHeartbeat > 10000){

    triggerFault(FAULT_LCD);

  }

  // ================= SENSOR =================
  if(now - lastSensorHeartbeat > 10000){

    triggerFault(FAULT_SENSOR);

  }

  // ================= CONTROL =================
  if(now - lastControlHeartbeat > 5000){

    triggerFault(FAULT_CONTROL);

  }

}

String faultName(FaultType type){

  switch(type){

    case FAULT_LCD:
      return "LCD";

    case FAULT_SENSOR:
      return "SENSOR";

    case FAULT_CONTROL:
      return "CONTROL";

    case FAULT_WIFI:
      return "WIFI";

    default:
      return "UNKNOWN";
  }
}


void triggerFault(FaultType type){

  // cegah spam
  if(faultActive) return;

  faultActive = true;

  waitingApproval = true;

  currentFault = type;

  Serial.println("FAULT DETECTED");

  switch(type){

    case FAULT_LCD:
      Serial.println("FAULT: LCD");
      break;

    case FAULT_SENSOR:
      Serial.println("FAULT: SENSOR");
      break;

    case FAULT_CONTROL:
      Serial.println("FAULT: CONTROL");
      break;

    case FAULT_WIFI:
      Serial.println("FAULT: WIFI");
      break;

    default:
      break;
  }

   // ================= TELEGRAM ALERT =================
  String msg;

msg += "╔════════════════╗\n";
msg += "⚠️ PERINGATAN SISTEM\n";
msg += "╚════════════════╝\n\n";

msg += "🔧 Modul Bermasalah\n";
msg += "└ " + faultName(type) + "\n\n";

msg += "📋 Status\n";
msg += "└ Sistem masih berjalan normal\n\n";

msg += "🛠 Tindakan\n";
msg += "└ Menunggu persetujuan recovery\n\n";

msg += "⏳ Auto recovery: 18:00 WITA\n\n";

msg += "👇 Tekan tombol di bawah";

  sendRecoveryApproval(msg);

}

void executeRecovery(){

  // ===== CEGAH DOUBLE RECOVERY =====
  if(!faultActive) return;


  Serial.println("RECOVERY START");

  faultActive = false;

  waitingApproval = false;

  String msg;

  msg += "╔════════════════╗\n";
  msg += "✅ RECOVERY BERHASIL\n";
  msg += "╚════════════════╝\n\n";

  msg += "🔧 Modul\n";
  msg += "└ " + faultName(currentFault) + "\n\n";

  msg += "⚙️ Perbaikan\n";
  msg += "└ Recovery otomatis dijalankan\n\n";

  msg += "📋 Status\n";
  msg += "└ Sistem kembali normal\n\n";

  msg += "🕒 Waktu\n";
  msg += "└ " + timeNow();

  sendTelegram(msg);

  currentFault = FAULT_NONE;

  recoveryRunning = false;
}