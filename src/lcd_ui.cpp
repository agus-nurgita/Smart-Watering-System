#include "lcd_ui.h"
#include "pins.h"
#include "global.h"
#include "time_utils.h"
#include "sensors.h"
#include "control.h"
#include "wifi_manager.h"
#include "hardware.h"

#include <WiFi.h>

int page = 0;
unsigned long lastPage = 0;

String lastRow0 = "";
String lastRow1 = "";

void lcdPrintRow(int row, String text){

  if(text.length() < 16){
    while(text.length() < 16){
      text += " ";
    }
  }

  if(row == 0){

    if(text == lastRow0) return;

    lcd.setCursor(0,0);
    lcd.print(text);
    lastRow0 = text;
  }

  else{

    if(text == lastRow1) return;

    lcd.setCursor(0,1);
    lcd.print(text);
    lastRow1 = text;
  }
}

String pad2(String s){
  if(s.length() == 0) return "  ";
  if(s.length() == 1) return s + " ";
  return s.substring(0,2);
}

void resetLcdCache(){
  lastRow0 = "";
  lastRow1 = "";
}

void runAnimation(){

  if(!animActive) return;

  if(millis() - animLastUpdate < 100) return; // kecepatan
  animLastUpdate = millis();

  static bool animFirst = true;

  if(animFirst){
    lcd.clear();
    resetLcdCache();
    animFirst = false;
  }

  // ===== MODE MUSIM =====
  if(animType == "SEASON"){

    if(animStep < 10){
      lcd.setCursor(0,0);
      lcd.print("MODE: " + animOldText);

      lcd.setCursor(0,1);
      for(int i=0;i<animStep;i++) lcd.print(">");
    }
    else if(animStep < 16){
      lcd.setCursor(0,0);
      lcd.print("MODE BARU");

      lcd.setCursor(0,1);
      lcd.print(animNewText);
    }
    else{
      animActive = false;
      animFirst = true;
      return;
    }
  }

  // ===== AUTO / MANUAL =====
  if(animType == "MODE"){

    if(animStep < 16){
      lcd.setCursor(0,0);
      lcd.print(animOldText);

      lcd.setCursor(0,1);
      for(int i=0;i<animStep;i++) lcd.print(".");
    }
    else if(animStep < 22){
      lcd.setCursor(0,0);
      lcd.print("MODE:");

      lcd.setCursor(0,1);
      lcd.print(animNewText);
    }
    else{
      animActive = false;
      animFirst = true;
      return;
    }
  }

  animStep++;
}

void printCenter(int row, String text){
  int start = (16 - text.length()) / 2;
  lcd.setCursor(start, row);
  lcdPrint16(text);
  }

void lcdAuto() {

  static int lastPageShown = -1;
  static int lastWaterState = -1;
  static bool lastNight = false;
  static bool lastReset = false;

  // ===== PRIORITY STATES =====
  if(resetBtnHolding){
    if(!lastReset){
      lcd.clear();
      resetLcdCache();
      lastReset = true;
    }
    lcdPrintRow(0, "TAHAN 5 DETIK");
    lcdPrintRow(1, "UNTUK RESTART");
    return;
  }
  lastReset = false;

  if(nightMode){
    if(!lastNight){
      lcd.clear();
      resetLcdCache();
      lastNight = true;
    }
    lcdPrintRow(0, "MODE MALAM");
    lcdPrintRow(1, "GOOD NIGHT");
    return;
  }
  lastNight = false;

  if(waterState == WATERING && remainPhase() > 0){
    if(lastWaterState != WATERING){
      lcd.clear();
      resetLcdCache();
      lastWaterState = WATERING;
    }
    lcdPrintRow(0, "AUTO SIRAM");
    lcdPrintRow(1, "SISA " + String(remainPhase()) + " dtk");
    return;
  }
  lastWaterState = IDLE;

  // ===== PAGE ROTATION =====
  if(millis() - lastPage > 3000){
    page = (page + 1) % 8;
    lastPage = millis();
  }

  // clear hanya kalau page berubah
  if(page != lastPageShown){
    lcd.clear();
    resetLcdCache();
    lastPageShown = page;
  }

  // ===== PAGE CONTENT =====
  switch(page){
    case 0:
      lcdPrintRow(0, "MODE: AUTO");
      lcdPrintRow(1, timeNow());
      break;
    
    case 1:
      {
        String modeLabel = "M Musim";
        String modeValue = String(getModeMusim());

        int colonPos = 7; // ':' di kolom ke-10 (index 0 = kolom 1)
        String row1 = modeLabel;
        
        while(row1.length() < colonPos) row1 += " ";
        row1 += ": " + modeValue;

        lcdPrintRow(0, row1);

        String wifiLabel = "WiFi";
        String ssid = WiFi.SSID();
        if(ssid.length() > 6) ssid = ssid.substring(0,6); // sesuaikan agar muat

        String row2 = wifiLabel;
        while(row2.length() < colonPos) row2 += " ";
        row2 += ": " + ssid;

       lcdPrintRow(1, row2);

      }
      break;

    case 2:
      lcdPrintRow(0, "CUACA");
      lcdPrintRow(1, "P:" + weatherP + " E:" + weatherE + " S:" + weatherS + " N:" + weatherN);
      break;
    
    case 3:
      lcdPrintRow(0, "RAIN SENSOR");
      lcdPrintRow(1, isRaining ? "HUJAN" : "TIDAK HUJAN");
      break;


    case 4:
      {
        char row1[17];
        char row2[17];

        // HEADER (jam)
        snprintf(row1, sizeof(row1), "%-4s%-4s%-4s%-4s",
                "06", "10", "14", "17");

        // DATA (status)
        snprintf(row2, sizeof(row2), "%-4s%-4s%-4s%-4s",
                pad2(getWaterStatus(6, schedule6)).c_str(),
                pad2(getWaterStatus(10, schedule10)).c_str(),
                pad2(getWaterStatus(14, schedule14)).c_str(),
                pad2(getWaterStatus(17, schedule17)).c_str());

        lcdPrintRow(0, String(row1));

        lcdPrintRow(1, String(row2));
      }
      break;

    case 5:
      lcdPrintRow(0, "T   H   TAN  S");

      lcdPrintRow(
        1,
        String((int)tAir) + "C " +
        String((int)hAir) + "% " +
        String(soilPercent) + "% " +
        String((int)tSoil) + "C"
      );
      break;  

    case 6:
      lcdPrintRow(0, "PUMP: " + String(pumpState ? "ON" : "OFF"));
      lcdPrintRow(1, "STANDBY");
      break;

    case 7: 
    lcdPrintRow(0, "HARIAN: " + String(dailyProgressPercent()) + "%");
    lcdPrintRow(1, String(dailyWaterDone) + "/" + String(dailyTargetCount()) + " SLOT");
    break;
  }
}
void lcdManual(){

  static int lastPageShown = -1;
  static bool lastNight = false;
  static bool lastReset = false;
  static bool lastPump = false;

  // ===== PRIORITY =====
  if(resetBtnHolding){
    if(!lastReset){
      lcd.clear();
      resetLcdCache();
      lastReset = true;
    }
    lcd.setCursor(0,0); lcdPrint16("TAHAN 5 DETIK");
    lcd.setCursor(0,1); lcdPrint16("UNTUK RESTART");
    return;
  }
  lastReset = false;

  if(nightMode){
    if(!lastNight){
      lcd.clear();
      resetLcdCache();
      lastNight = true;
    }
    lcd.setCursor(0,0); lcdPrint16("MODE MALAM");
    lcd.setCursor(0,1); lcdPrint16("GOOD NIGHT");
    return;
  }
  lastNight = false;

  if(pumpState){
    if(!lastPump){
      lcd.clear();
      resetLcdCache();
      lastPump = true;
    }
    lcdPrintRow(0, "MANUAL SIRAM");
    lcdPrintRow(1, "ON " + String((millis()-pumpStartManual)/1000) + " dtk");
    return;
  }
  lastPump = false;

  // ===== PAGE ROTATION =====
  if(millis() - lastPage > 3000){
    page = (page + 1) % 4;
    lastPage = millis();
  }

  if(page != lastPageShown){
    lcd.clear();
    resetLcdCache();
    lastPageShown = page;
  }

  switch(page){
    case 0:
      lcdPrintRow(0, "MODE: MANUAL");
      lcdPrintRow(1, timeNow());
      break;

    case 1:
      {
        String modeLabel = "M Musim";
        String modeValue = String(getModeMusim());

        int colonPos = 7; // ':' di kolom ke-10 (index 0 = kolom 1)
        String row1 = modeLabel;
        
        while(row1.length() < colonPos) row1 += " ";
        row1 += ": " + modeValue;

        lcdPrintRow(0, row1);

        String wifiLabel = "WiFi";
        String ssid = WiFi.SSID();
        if(ssid.length() > 6) ssid = ssid.substring(0,6); // sesuaikan agar muat

        String row2 = wifiLabel;
        while(row2.length() < colonPos) row2 += " ";
        row2 += ": " + ssid;

        lcdPrintRow(1, row2);

      }
      break;  

    case 2:
      lcdPrintRow(0, "T   H   TAN  S");

      lcdPrintRow(
        1,
        String((int)tAir) + "C " +
        String((int)hAir) + "% " +
        String(soilPercent) + "% " +
        String((int)tSoil) + "C"
      );
      break;

    case 3:
      lcdPrintRow(0, "PUMP: OFF");
      lcdPrintRow(1, "SIAP SIRAM");
      break;

  }
}

void lcdTask(){

  if(justWokeUp){
    lcd.clear();
    resetLcdCache();
    justWokeUp = false;
  }

  if(resetBtnHolding) return;
  if(otaActive) return;

  // 🔥 PRIORITAS ANIMASI
  if(animActive){
    runAnimation();
    return;
  }

  if(manualMode) lcdManual();
  else lcdAuto();

  lastLCDHeartbeat = millis();
}

void nightManager(){
  static bool lastState = false;

  bool nowNight = isNight();

  if(nowNight != lastState){
    nightMode = nowNight;

    if(nightMode){
      // MASUK MODE MALAM
      WiFi.setSleep(true);
      lcd.noBacklight();
      pumpState = false;
      digitalWrite(RELAY_PIN, HIGH);
    } 

    else {
      // KELUAR MODE MALAM
      WiFi.setSleep(false);

      lcd.backlight();
      lcd.clear();
      resetLcdCache();
      justWokeUp = true;
    }

    lastState = nowNight;
  }
}
