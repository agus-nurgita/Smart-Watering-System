#ifndef HARDWARE_H
#define HARDWARE_H

#include <DHT.h>
#include <DallasTemperature.h>
#include <Adafruit_ADS1X15.h>
#include <LiquidCrystal_I2C.h>

// object extern
extern LiquidCrystal_I2C lcd;
extern DHT dht;
extern DallasTemperature ds;
extern Adafruit_ADS1115 ads;

void initHardware();

#endif