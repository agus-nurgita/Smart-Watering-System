#ifndef CONTROL_H
#define CONTROL_H

#include <Arduino.h>
#include "global.h"

// ===== FUNCTION =====
void systemControl();
void startWater(unsigned long duration);

int getEstimatedDuration(int h);
int getAutoWaterDurationAI(int slotHour);
bool allowSmartWatering(int h);

unsigned long remainPhase();
String getWaterStatus(int h, bool enabled);
int progressPercent();
int dailyTargetCount();
int dailyProgressPercent();
void checkResetButton();
void autoRestartTask();

String evaluateSmartDecisionReasonPretty(int h);
String soilCondition();
String buildDailyWaterReport();
void stopAllSystem();

void setupTask();

void watchdogMonitor();
void triggerFault(FaultType type);
void executeRecovery();


#endif