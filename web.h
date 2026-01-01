#pragma once
#include <WebServer.h>

extern WebServer server;
extern String inputBuffer;
extern bool newCommandFromWeb;

void initWebServer();
void handleRoot();
void handleCommand();
void handleOutput();
