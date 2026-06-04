#ifndef WEATHER_H
#define WEATHER_H

bool isForecastRainAtHour(int h);
bool isForecastRainAtHour(int hour);
void fetchWeather();
String weatherCode(String desc);
void weatherScheduler();

#endif