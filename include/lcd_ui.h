#ifndef LCD_UI_H
#define LCD_UI_H

#include <Arduino.h>

void lcdPrintRow(int row, String text);
void lcdTask();
void lcdAuto();
void lcdManual();
void nightManager();

void runAnimation();
void printCenter(int row, String text);
void resetLcdCache();

#endif