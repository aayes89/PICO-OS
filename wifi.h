#pragma once
#include <WiFi.h>
#include <HTTPClient.h>

#define HOSTNAME "PICO-OS"

void loadWiFiConfig();
// Auxiliares
const char* encToString(uint8_t enc);
const char* macToString(uint8_t mac[6]);