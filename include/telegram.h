#ifndef TELEGRAM_H
#define TELEGRAM_H

void sendTelegram(String message, String chatID);
void checkTelegramMessage();
void telegramScheduler();
String buildStatusMessage();
void preWaterNotification();
void rainAlertTask();
void sendWateringReport(int hourSlot, int duration);
void morningSync();
void sendPendingReports();
void sendRecoveryApproval(String msg);

#endif