#include "time_utils.h"
#include <time.h>

int hourNow(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) return -1;
  return timeinfo.tm_hour;
}

int minNow(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) return -1;
  return timeinfo.tm_min;
}

int dayNow(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) return -1;
  return timeinfo.tm_mday;
}

bool isNight(){

  int h = hourNow();

  // waktu belum sync
  if(h == -1){
    return false;
  }

  return (h >= 19 || h < 5);
}

String timeNow(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) return "--:--";

  char buffer[10];
  sprintf(buffer,"%02d:%02d:%02d",
          timeinfo.tm_hour,
          timeinfo.tm_min,
          timeinfo.tm_sec);

  return String(buffer);
}