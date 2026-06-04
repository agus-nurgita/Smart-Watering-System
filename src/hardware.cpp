#include "hardware.h"
#include "pins.h"
#include "telegram.h"
#include "global.h"

#include <OneWire.h>

// instance asli
LiquidCrystal_I2C lcd(0x27,16,2);
DHT dht(DHTPIN, DHTTYPE);

OneWire oneWire(DS18B20_PIN);
DallasTemperature ds(&oneWire);

Adafruit_ADS1115 ads;

void initHardware() {
  lcd.init();
  lcd.backlight();
}