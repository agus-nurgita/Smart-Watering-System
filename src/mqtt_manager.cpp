#include "mqtt_manager.h"

#include "global.h"
#include "sensors.h"

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ======================
// MQTT CONFIG
// ======================

String mqttServer = "greenhouse.local";
const int mqttPort = 1883;

// ======================
// MQTT CLIENT
// ======================

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// ======================
// MQTT CALLBACK
// ======================

void mqttCallback(char* topic, byte* payload, unsigned int length){

    String message;

    for(unsigned int i = 0; i < length; i++){
        message += (char)payload[i];
    }

    Serial.println("MQTT Message:");
    Serial.println(topic);
    Serial.println(message);

    // =========================
    // HISTORY URL
    // =========================

    if(String(topic) == "greenhouse/server/history_url"){

        historyURL = message;

        Serial.println("History URL Updated:");
        Serial.println(historyURL);
    }
}

// ======================
// MQTT CONNECT
// ======================

void reconnectMQTT(){

    static unsigned long lastTry = 0;

    // retry tiap 5 detik
    if(millis() - lastTry < 5000) return;

    lastTry = millis();

    Serial.println("MQTT connecting...");

    String clientId = "ESP32-" + String(random(0xffff), HEX);

    if(mqttClient.connect(clientId.c_str())){

        Serial.println("✅ MQTT connected");


        mqttClient.subscribe("greenhouse/server/history_url");

        Serial.println("✅ Subscribed history URL");
    }
    else{

        Serial.print("❌ MQTT failed: ");
        Serial.println(mqttClient.state());
    }
}

// ======================
// INIT MQTT
// ======================

void initMQTT(){

    mqttClient.setServer(mqttServer.c_str(), mqttPort);

    mqttClient.setCallback(mqttCallback);
}

// ======================
// RUN MQTT
// ======================

void runMQTT(){

    if(WiFi.status() != WL_CONNECTED) return;

    if(!mqttClient.connected()){
        reconnectMQTT();
    }

    mqttClient.loop();
}

// ======================
// PUBLISH SENSOR DATA
// ======================

void publishSensorData(){

    static unsigned long lastPublish = 0;

    if(millis() - lastPublish < 5000) return;

    lastPublish = millis();

    DynamicJsonDocument doc(256);

    doc["temp"] = tAir;
    doc["humidity"] = hAir;

    doc["soil"] = soilPercent;
    doc["soil_temp"] = tSoil;

    doc["pump"] = pumpState ? 1 : 0;
    doc["rain"] = rainDetectedRaw ? 1 : 0;

    String payload;
    serializeJson(doc, payload);

    if(!mqttClient.connected()) return;

    mqttClient.publish("greenhouse/data", payload.c_str());

    Serial.println("📤 MQTT: " + payload);
}

void updateMQTTServer(String ip){

  mqttServer = ip;

  mqttClient.setServer(mqttServer.c_str(), 1883);

  Serial.println("MQTT Server Updated:");
  Serial.println(mqttServer);
}