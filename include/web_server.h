#ifndef WEB_SERVER_H
#define WEB_SERVER_H


void handleLogin();
void setupWebServer();
void initWebServer();
void handleMQTTIP();
void handleHistoryURL();
bool isAuthenticated();

void handleData();
void handleMode();
void handleSeason();
void handlePump();
void handleSchedule();
void handleUpload();

void handleHistory();
void handleReport();

void logHourlyData();
void resetDailyLog();
void handleLogout();
void cleanupSessions();
void sessionMaintenance();
void handleDebugToken();


#endif