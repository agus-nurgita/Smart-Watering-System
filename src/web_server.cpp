#include "web_server.h"

#include "global.h"
#include "time_utils.h"
#include "control.h"
#include "sensors.h"
#include "weather.h"
#include "mqtt_manager.h"

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <WebServer.h>

#define LOG_FILE "/log.csv"

const String SECURITY_KEY = "GREENHOUSE2026";

extern File uploadFile;
File uploadFile;

// ======================
// Web upload file
// ======================
const char* uploadPage = R"rawliteral(
<!DOCTYPE html>
<html>
<body>
<h2>Upload Files</h2>

<input type="file" id="files" multiple>
<button onclick="uploadFiles()">Upload</button>

<p id="status"></p>

<script>
async function uploadFiles() {
  const files = document.getElementById('files').files;
  const status = document.getElementById('status');

  if (files.length === 0) {
    status.innerHTML = "Pilih file dulu!";
    return;
  }

  for (let i = 0; i < files.length; i++) {
    const formData = new FormData();
    formData.append("data", files[i]);

    status.innerHTML = "Upload: " + files[i].name;

    await fetch('/upload', {
      method: 'POST',
      body: formData
    });
  }

  status.innerHTML = "Semua file berhasil diupload!";
}
</script>

</body>
</html>
)rawliteral";

bool checkApiKey()
{
    if (!server.hasArg("key"))
    {
        server.send(403, "text/plain", "Missing API Key");
        return false;
    }

    if (server.arg("key") != SECURITY_KEY)
    {
        server.send(403, "text/plain", "Invalid API Key");
        return false;
    }

    return true;
}

void handleMQTTIP(){

 if (!checkApiKey()) return;

 if (server.hasArg("ip")){
    String ip = server.arg("ip");

    updateMQTTServer(ip);

    server.send(200, "text/plain", "MQTT IP Updated");
    return;

 }

    server.send(400, "text/plain", "No IP");

}

void handleHistoryURL()
{
    if (!checkApiKey())
        return;

    if(server.hasArg("url"))
    {
        historyURL = server.arg("url");

        Serial.println("History URL Updated:");
        Serial.println(historyURL);

        server.send(200, "text/plain", "History URL Saved");
        return;
    }

    server.send(400, "text/plain", "No URL");
}



void setupWebServer() {

  server.on("/upload", HTTP_GET, []() {
    server.send(200, "text/html", uploadPage);
  });

  server.on("/upload", HTTP_POST, []() {
    server.send(200, "text/plain", "Upload OK");
  }, handleUpload);

  server.on("/setmqtt", handleMQTTIP);
  server.on("/sethistory", handleHistoryURL);
  server.on("/logout", HTTP_POST, handleLogout);

}

void initWebServer(){

  server.on("/", [](){
    File file = LittleFS.open("/index.html", "r");
    server.streamFile(file, "text/html");
    file.close();
  });

  server.on("/script.js", [](){
    File file = LittleFS.open("/script.js", "r");
    server.streamFile(file, "application/javascript");
    file.close();
  });

  server.on("/style.css", [](){
    File file = LittleFS.open("/style.css", "r");
    server.streamFile(file, "text/css");
    file.close();
  });

  server.on("/login", HTTP_POST, handleLogin);

  server.on("/mode", handleMode);
  server.on("/pump", handlePump);
  server.on("/data", handleData);
  server.on("/season", handleSeason);
  server.on("/schedule", handleSchedule);
  server.on("/history", handleHistory);
  server.on("/report", handleReport);

  server.onNotFound([]() {

    if(server.method() == HTTP_OPTIONS){

      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
      server.sendHeader("Access-Control-Allow-Headers", "*");

      server.send(200);

    } else {

      server.send(404, "text/plain", "Not found");

    }
  });

  setupWebServer();

  server.begin();
}

void handleLogout()
{
    if(!server.hasArg("token"))
    {
        server.send(401, "text/plain", "Unauthorized");
        return;
    }

    String token = server.arg("token");

    for(int i = 0; i < MAX_SESSIONS; i++)
    {
        if(sessionTokens[i] == token)
        {
            sessionTokens[i] = "";
            sessionTimes[i] = 0;

            server.send(200, "text/plain", "Logged out");
            return;
        }
    }

    server.send(401, "text/plain", "Unauthorized");
}

void cleanupSessions(){
    for(int i = 0; i < MAX_SESSIONS; i++)
    {
        if(sessionTokens[i] != "")
        {
            if(millis() - sessionTimes[i] > SESSION_TIMEOUT)
            {
                sessionTokens[i] = "";
                sessionTimes[i] = 0;
            }
        }
    }
}

// ======================
// HANDLE DATA (API)
// ======================
void handleData() {

  if(!isAuthenticated()){
    server.send(401, "text/plain", "Unauthorized");
    return;
  }

  DynamicJsonDocument doc(512);

  doc["tAir"] = tAir;
  doc["hAir"] = hAir;
  doc["tSoil"] = tSoil;
  doc["soil"] = soilPercent;
  doc["pump"] = pumpState ? 1 : 0;
  doc["mode"] = manualMode ? "manual" : "auto";
  doc["season"] = seasonMode;

  doc["s6"] = schedule6;
  doc["s10"] = schedule10;
  doc["s14"] = schedule14;
  doc["s17"] = schedule17;

  doc["rain"] = rainPercent;
  doc["rainStatus"] = isRaining ? "rain" : "dry";

  doc["wP"] = weatherP;
  doc["wE"] = weatherE;
  doc["wS"] = weatherS;
  doc["wN"] = weatherN;

  doc["dailyDone"] = dailyWaterDone;
  doc["dailyTarget"] = dailyTargetCount();
  doc["dailyPercent"] = dailyProgressPercent();
  doc["historyURL"] = historyURL;

  String json;
  serializeJson(doc, json);

  server.sendHeader("Access-Control-Allow-Origin", "*"); // 🔥 penting untuk web
  server.send(200, "application/json", json);
}

bool isAuthenticated(){
    if(!server.hasArg("token")){
        return false;
    }

    String token = server.arg("token");

    for(int i = 0; i < MAX_SESSIONS; i++)
    {
        if(sessionTokens[i] == token){
            if(millis() - sessionTimes[i] > SESSION_TIMEOUT)
            {
                sessionTokens[i] = "";
                sessionTimes[i] = 0;
                return false;
            }

            sessionTimes[i] = millis(); // refresh session
            return true;
        }
    }

    return false;
}

void handleLogin(){

  if(millis() < loginBlockedUntil){
      server.send(429, "text/plain", "Too many login attempts");
      return;
  }
  
  if(!server.hasArg("username") || !server.hasArg("password")){

    server.send(400, "text/plain", "Missing username or password");
    return;

  }

  String user = server.arg("username");
  String pass = server.arg("password");

  if(user == adminUsername && pass == adminPassword){

    String newToken = String(millis()) + "_GH";

    bool slotFound = false;

    for(int i = 0; i < MAX_SESSIONS; i++)
    {
        if(sessionTokens[i] == "")
        {
            sessionTokens[i] = newToken;
            sessionTimes[i] = millis();

            slotFound = true;
            break;
        }
    }

    if(!slotFound)
    {
        server.send(503,
                    "text/plain",
                    "Maximum sessions reached");
        return;
    }

    loginFailCount = 0;

    DynamicJsonDocument doc(256);

    doc["success"] = true;
    doc["token"] = newToken;

    String json;
    serializeJson(doc, json);

    server.send(200, "application/json", json);
    return;
  }

    if(millis() < loginBlockedUntil)
  {
      server.send(429,
                  "text/plain",
                  "Too many login attempts");

      return;
  }

    loginFailCount++;

  if(loginFailCount >= 3)
  {
      loginBlockedUntil = millis() + 60000UL; // 1 menit
      loginFailCount = 0;
  }

  server.send(401, "text/plain", "Invalid username or password");
}

void handleMode(){

  if(!isAuthenticated())
    {
        server.send(401, "text/plain", "Unauthorized");
        return;
    }

    server.sendHeader("Access-Control-Allow-Origin", "*");

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "*");

  if(server.hasArg("mode")){
    String m = server.arg("mode");

    if(m == "auto"){
      manualMode = false;
    }
    else if(m == "manual"){
      manualMode = true;
    }

    Serial.println("Mode Web: " + m);
  }

  server.send(200,"text/plain","OK");
}

void handleSeason(){

  if(!isAuthenticated())
    {
        server.send(401, "text/plain", "Unauthorized");
        return;
    }

    server.sendHeader("Access-Control-Allow-Origin", "*");

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "*");

  if(server.hasArg("season")){
    String s = server.arg("season");

    if(s == "normal"){
      seasonMode = 0;
    }
    else if(s == "kemarau"){
      seasonMode = 1;
    }
    else if(s == "hujan"){
      seasonMode = 2;
    }

    Serial.println("Season Web: " + s);
  }

  server.send(200,"text/plain","OK");
}

void handlePump(){

  if(!isAuthenticated())
    {
        server.send(401, "text/plain", "Unauthorized");
        return;
    }

    server.sendHeader("Access-Control-Allow-Origin", "*");

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "*");

  if(!manualMode){
    Serial.println("IGNORED (AUTO MODE)");
    server.send(200,"text/plain","AUTO MODE");
    return;
  }

  if(server.hasArg("state")){
    pumpState = server.arg("state").toInt();
    pumpManual = pumpState;

    if(pumpManual){
      pumpStartManual = millis();
    }

    Serial.println("Pump Manual: " + String(pumpManual));
  }

  server.send(200,"text/plain","OK");
}

void handleSchedule() {

  if(!isAuthenticated())
    {
        server.send(401, "text/plain", "Unauthorized");
        return;
    }

  server.sendHeader("Access-Control-Allow-Origin", "*");

  if(server.hasArg("slot") && server.hasArg("state")){

    int slot = server.arg("slot").toInt();
    bool state = server.arg("state").toInt();

    if(slot == 6) schedule6 = state;
    else if(slot == 10) schedule10 = state;
    else if(slot == 14) schedule14 = state;
    else if(slot == 17) schedule17 = state;

    Serial.println("Schedule " + String(slot) + " = " + String(state));
  }

  server.send(200, "text/plain", "OK");
}

void handleUpload() {
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    String filename = "/" + upload.filename;
    uploadFile = LittleFS.open(filename, "w");
  } 
  else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) uploadFile.write(upload.buf, upload.currentSize);
  } 
  else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) uploadFile.close();
  }
}

// =========================
// 📊 MODULE: DATA LOGGER
// =========================

// --- FUNCTION: logHourlyData ---
// Description: simpan data setiap jam ke LittleFS
void logHourlyData(){

  int h = hourNow();

  // cegah double save dalam 1 jam
  if(h == lastLoggedHour) return;

  lastLoggedHour = h;

  File file = LittleFS.open(LOG_FILE, FILE_APPEND);

  if(!file){
    Serial.println("Gagal buka file log");
    return;
  }

  // format CSV
  // timestamp,tAir,hAir,tSoil,soil,adc
  String data = "";

  data += timeNow(); data += ",";
  data += String(tAir); data += ",";
  data += String(hAir); data += ",";
  data += String(tSoil); data += ",";
  data += String(soilPercent); data += ",";
  data += String(soilRaw);

  data += "\n";

  file.print(data);
  file.close();

  Serial.println("✅ Data logged: " + data);
}

// =========================
// 🌐 API: HISTORY DATA
// =========================
void handleHistory(){

  if(!isAuthenticated()){
      server.send(401, "text/plain", "Unauthorized");
      return;
  }

  File file = LittleFS.open(LOG_FILE, "r");

  if(!file){
    server.send(200, "application/json", "[]");
    return;
  }

  String json = "[";
  bool first = true;

  while(file.available()){

    String line = file.readStringUntil('\n');

    if(line.length() < 5) continue;

    // parsing CSV
    int i1 = line.indexOf(',');
    int i2 = line.indexOf(',', i1+1);
    int i3 = line.indexOf(',', i2+1);
    int i4 = line.indexOf(',', i3+1);
    int i5 = line.indexOf(',', i4+1);

    String t  = line.substring(0, i1);
    String ta = line.substring(i1+1, i2);
    String ha = line.substring(i2+1, i3);
    String ts = line.substring(i3+1, i4);
    String sm = line.substring(i4+1, i5);
    String adc= line.substring(i5+1);

    if(!first) json += ",";
    first = false;

    json += "{";
    json += "\"time\":\"" + t + "\",";
    json += "\"tAir\":" + ta + ",";
    json += "\"hAir\":" + ha + ",";
    json += "\"tSoil\":" + ts + ",";
    json += "\"soil\":" + sm + ",";
    json += "\"adc\":" + adc;
    json += "}";
  }

  json += "]";

  file.close();

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

// =========================
// 🌐 API: REPORT DATA
// =========================
void handleReport(){

  if(!isAuthenticated()){
      server.send(401, "text/plain", "Unauthorized");
      return;
  }

  // sementara sama dengan history
  // nanti bisa ditambah filter harian/mingguan

  handleHistory();
}

// =========================
// 🧹 RESET LOG HARIAN
// =========================
void resetDailyLog(){

  static int lastDayLogged = -1;

  int today = dayNow();

  if(today != lastDayLogged){

    LittleFS.remove(LOG_FILE); // hapus file lama
    lastDayLogged = today;

    Serial.println("🗑 Log reset harian");
  }
}
