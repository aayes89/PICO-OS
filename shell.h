#pragma once

#define MAX_CMD_LEN 128    // Máximo longitud de comando

void printPrompt();
void executeCommand(char* input);
int tokenize(char* input, char* argv[], int max);