#include "shell.h"
#include "commands.h"
#include "io.h"

#define MAX_ARGS 16

// =============================================
//  Muestra el prompt con path actual
// =============================================
void printPrompt() {
  Serial.print(currentPath);
  Serial.print("> ");
}

int tokenize(char* input,char* argv[],int max){
  int argc=0; char* p=input;
  while(*p && argc<max){
    while(*p==' '||*p=='\t') p++;
    if(!*p) break;
    if(*p=='"'){ p++; argv[argc++]=p; while(*p&&*p!='"') p++; if(*p)*p++=0; }
    else { argv[argc++]=p; while(*p&&*p!=' '&&*p!='\t') p++; if(*p)*p++=0; }
  }
  argv[argc]=nullptr; return argc;
}
// =============================================
//  Ejecuta el comando ingresado
// =============================================
void executeCommand(char* input) {
  // Separa en tokens
  char* argv[MAX_ARGS + 1];
  int argc = tokenize(input, argv, MAX_ARGS);
  if (argc == 0) return;
  // Busca y ejecuta
  bool found = false;
  for (int i = 0; commands[i].name != nullptr; i++) {
    if (strcmp(argv[0], commands[i].name) == 0) {
      commands[i].func(argc, argv);
      found = true;
      break;
    }
  }
  if (!found) {
    outPrint("Comando no reconocido: ");
    outPrintln(argv[0]);
  }
}