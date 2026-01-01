#pragma once
#include <Arduino.h>
#ifndef RP2040
  #define LED_PIN 25
#else
  #define LED_PIN "LED"
#endif

extern String currentPath;
extern String outputBuffer;
extern bool isWebCommand;

void outPrint(const char* s);
void outPrint(const String& s);
void outPrintln(const char* s = "");
void outPrintln(const String& s);
void outPrintf(const char* fmt, ...);

void console_log(const char* arg);
void sanitizeLine(char* s);
