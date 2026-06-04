#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <Arduino.h>

int hourNow();
int minNow();
int dayNow();

bool isNight();
String timeNow();

#endif