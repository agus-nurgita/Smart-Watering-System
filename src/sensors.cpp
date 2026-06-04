#include "sensors.h"
#include "config.h"
#include "pins.h"
#include "global.h"
#include "hardware.h"
#include "telegram.h"

#include <Arduino.h>
#include <DallasTemperature.h>

// ======================
// SOIL FILTER
// ======================
int soilBuffer[SOIL_SAMPLE_COUNT];
int soilIndex = 0;
bool soilBufferFilled = false;


// ======================
// HELPER FUNCTION
// ======================
int getMedian(int *arr, int size){
  int temp[size];
  memcpy(temp, arr, size * sizeof(int));

  for(int i = 0; i < size-1; i++){
    for(int j = i+1; j < size; j++){
      if(temp[j] < temp[i]){
        int t = temp[i];
        temp[i] = temp[j];
        temp[j] = t;
      }
    }
  }

  return temp[size/2];
}

// ======================
// SENSOR READING (FINAL CLEAN)
// ======================
void readSensor(){


  lastSensorHeartbeat = millis();

  // ===== DHT (AIR) =====
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if(!isnan(t)) tAir = t;
  if(!isnan(h)) hAir = h;


  // ===== DS18B20 (SOIL TEMP) =====
  ds.requestTemperatures();
  float tempSoil = ds.getTempCByIndex(0);

  if(tempSoil != DEVICE_DISCONNECTED_C){
    tSoil = tempSoil;
    tSoilRaw = tempSoil;
  }


  // ===== SOIL MOISTURE (ADS1115) =====
  if(USE_SOIL_SENSOR){

    int samples[SOIL_SAMPLE_COUNT];

    for(int i = 0; i < SOIL_SAMPLE_COUNT; i++){
      samples[i] = ads.readADC_SingleEnded(0);
      delay(3);
    }

    // ===== MEDIAN FILTER =====
    int adcMedian = getMedian(samples, SOIL_SAMPLE_COUNT);
    soilRaw = adcMedian;
    soilFiltered = (soilFiltered * 0.8) + (soilRaw * 0.2);

    // ===== KONVERSI KE PERSEN =====
    float soilPercentF =
      (float)(SOIL_ADC_MAX - soilFiltered) /
      (float)(SOIL_ADC_MAX - SOIL_ADC_MIN) * 100.0;

      soilPercent = constrain((int)soilPercentF, 0, 100);
  }

  // ===== RAIN SENSOR (ANALOG via ADS1115) =====
  if(USE_ANALOG_RAIN){

    int rSamples[5];
    for(int i=0;i<5;i++){
      rSamples[i] = ads.readADC_SingleEnded(1);
      delay(2);  // kecil saja
    }

    rainRaw = getMedian(rSamples, 5);  // reuse fungsi kamu

    // ===== HYSTERESIS =====
    rainDetectedRaw = (rainRaw < RAIN_THRESHOLD);
    
  }

  Serial.print("RAIN RAW: ");
  Serial.print(rainRaw);
  Serial.print(" | STATE: ");
  Serial.println(rainDetectedRaw ? "BASAH" : "KERING");

}

void rainStabilityFilter(){
  Serial.println(rainRaw);

  // kalau kondisi berubah → mulai hitung waktu
  if(rainDetectedRaw != rainStableState){

    if(rainChangeTimer == 0){
      rainChangeTimer = millis();
    }

    // kalau sudah stabil 30 detik → baru update
    if(millis() - rainChangeTimer >= RAIN_STABLE_TIME){
      rainStableState = rainDetectedRaw;
      rainChangeTimer = 0;
    }

  } else {
    rainChangeTimer = 0;
  }

  // hasil final
  isRaining = rainStableState;
}