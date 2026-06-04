#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>

extern String mqttServer;

void initMQTT();
void runMQTT();
void publishSensorData();
void updateMQTTServer(String ip);
void mqttCallback(char* topic, byte* payload, unsigned int length);

#endif