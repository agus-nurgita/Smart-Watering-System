#ifndef CONFIG_H
#define CONFIG_H

#define BLYNK_TEMPLATE_ID "TMPL6spPV1VTK"
#define BLYNK_TEMPLATE_NAME "Plant Watering System"
#define BLYNK_AUTH_TOKEN "ISRISe3Sz9l5shhUk33ATZ1S6tf5n83U"

// ======================
// CONFIG & DEFINE
// ======================
//token web dasbord max 4 session
#define MAX_SESSIONS 4
//token telegram
#define TELEGRAM_TOKEN "8550373062:AAH4eUc_aANSHemje11brGwxUksDMl4qDM4"  // ganti dengan token botmu
#define TELEGRAM_CHAT_ID "5826535516"                             // ganti dengan chat ID tujuan
// batas ADC sensor
#define SOIL_ADC_MAX 12000
#define SOIL_ADC_MIN 6000
//kelembaban tanah
#define USE_SOIL_SENSOR true
// cek data adc untuk penyiraman
#define SOIL_SAMPLE_COUNT 10
// lokasi ramalan cuaca
#define LAT "-8.54612551791665"
#define LON "115.08235999970162"
#define API_KEY "fc38bfe9ce9bd29dca39407254a6b398"
// DATA LOGGER CONFIG
#define LOG_FILE "/log.csv"
// ===== RAIN ANALOG SENSOR (SOIL BASED) =====
#define USE_ANALOG_RAIN false
#define RAIN_ADC_CHANNEL 1   // ADS1115 channel (A1 misalnya)
#define RAIN_THRESHOLD 9500  // kalibrasi nanti!
#define RAIN_STABLE_TIME 30000UL  // 30 detik

#endif