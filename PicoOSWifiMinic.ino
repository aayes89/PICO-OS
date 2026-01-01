// =============================================
//  Shell Avanzado para Raspberry Pi Pico - Arduino Core
//  BusyBox-like + Filesystem con LittleFS + Wifi + WebServer
//  Usa Serial a 115200 baudios
//  Autor: Slam 2025
// =============================================
#include "io.h"
#include "fs.h"
#include "wifi.h"
#include "web.h"
#include "shell.h"
#include "commands.h"

unsigned long startTime;   // Para calcular uptime

void setup() {
  console_log("Cargando...\n");
  console_log("Iniciando PICO-OS V1...\n");
  console_log("Habilitando UART...\n");
  Serial.begin(115200);
  while (!Serial) delay(10);  // Espera a que se abra el monitor
  console_log("UART = 115200 bauds\n");
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  startTime = millis();

  console_log("Iniciando sistema de archivos LittleFS...\n");
  initFS();  // Inicializa LittleFS

  //console_log("Iniciando servicio TinyPython...\n");
  //initTinyPy();

  console_log("Iniciando módulo WiFi...\n");
  //WiFi.mode(CYW43_ITF_STA); // WIFI_STA
  WiFi.setHostname(HOSTNAME);
  WiFi.disconnect();
  console_log("Cargando configuración WiFi...\n");
  loadWiFiConfig();
  console_log("Módulo WiFi iniciado en STA\n");
  console_log("Iniciando servicios web...\n");
  initWebServer();  // Inicializa servidor web local
  
  Serial.printf("\n=====================================\n");
  Serial.printf("   Shell Pico-OS (BusyBox-like) \n");
  Serial.printf("   Escribe 'help' para comandos\n");
  Serial.printf("=====================================\n");
  printPrompt();
}

void loop() {
  server.handleClient();  // Procesa peticiones web

  // Procesar comandos vía serial o web
  static char cmdBuffer[MAX_CMD_LEN];
  static int bufPos = 0;
  static uint32_t t = 0;
  // Entrada vía serial
  if (Serial.available()) {
    char c = Serial.read();
    // Echo local
    Serial.print(c);
    if (c == '\r' || c == '\n') {
      Serial.println();
      cmdBuffer[bufPos] = '\0';
      if (bufPos > 0) {
        executeCommand(cmdBuffer);
      }
      bufPos = 0;
      printPrompt();
    } else if (c == 127 || c == '\b') {  // Backspace
      if (bufPos > 0) {
        bufPos--;
        Serial.print("\b \b");
      }
    } else if (bufPos < MAX_CMD_LEN - 1) {
      cmdBuffer[bufPos++] = c;
    }
  }
  // Entrada vía web
  if (newCommandFromWeb && inputBuffer.length() > 0) {
    // Copiar con terminación segura
    size_t n = inputBuffer.length();
    if (n >= MAX_CMD_LEN) n = MAX_CMD_LEN - 1;
    memcpy(cmdBuffer, inputBuffer.c_str(), n);
    cmdBuffer[n] = '\0';
    sanitizeLine(cmdBuffer);
    // Echo hacia la respuesta web
    outputBuffer += currentPath;
    outputBuffer += "> ";
    outputBuffer += cmdBuffer;
    outputBuffer += "\n";
    isWebCommand = true;
    executeCommand(cmdBuffer);
    isWebCommand = false;
    newCommandFromWeb = false;
    inputBuffer = "";
  }
  if (millis() - t > 1000) {
    flushFS();
    t = millis();
  }
}
