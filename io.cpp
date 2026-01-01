#include "io.h"

String currentPath = "/";
String outputBuffer = "";
bool isWebCommand = false;

void outPrint(const char* s) {
  if (isWebCommand) outputBuffer += s;
  else Serial.print(s);
}
void outPrint(const String& s) {
  outPrint(s.c_str());
}
void outPrintln(const char* s) {
  outPrint(s);
  outPrint("\n");
}
void outPrintln(const String& s) {
  outPrint(s);
  outPrint("\n");
}
void outPrintf(const char* fmt, ...) {
  char buf[256];
  va_list a;
  va_start(a, fmt);
  vsnprintf(buf, sizeof(buf), fmt, a);
  va_end(a);
  outPrint(buf);
}
void console_log(const char* arg) {
  Serial.printf(arg);
  delay(2000);
}
void sanitizeLine(char* s) {
  // Trim right: espacios y CR/LF
  int n = strlen(s);
  while (n > 0 && (s[n - 1] == ' ' || s[n - 1] == '\r' || s[n - 1] == '\n' || s[n - 1] == '\t')) {
    s[--n] = '\0';
  }
  // Trim left (opcional)
  while (*s == ' ' || *s == '\t') {
    memmove(s, s + 1, strlen(s));
  }
}
