#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <WebServer.h>

#define PORTAL_SSID "esp32_f871FE68"

void openWiFiPortal();
void checkWiFi();
void stopAllSystem();
void wifiReconnectTask();
void setupOTA();

#endif