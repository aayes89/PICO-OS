#pragma once

// Estructura para comandos
struct Command {
  const char* name;
  void (*func)(int argc, char* argv[]);
  const char* help;
};
extern Command commands[];

void registerCommands();   // opcional si prefieres construir dinámico

// =============================================
//  Prototipos de funciones de comandos
// =============================================
void cmd_help(int argc, char* argv[]);
void cmd_echo(int argc, char* argv[]);
void cmd_clear(int argc, char* argv[]);
void cmd_uptime(int argc, char* argv[]);
void cmd_led(int argc, char* argv[]);
void cmd_ls(int argc, char* argv[]);
void cmd_mkdir(int argc, char* argv[]);
void cmd_cd(int argc, char* argv[]);
void cmd_pwd(int argc, char* argv[]);
void cmd_touch(int argc, char* argv[]);
void cmd_rm(int argc, char* argv[]);
void cmd_cat(int argc, char* argv[]);
void cmd_write(int argc, char* argv[]);  // echo "text" > file
void cmd_cp(int argc, char* argv[]);
void cmd_mv(int argc, char* argv[]);
void cmd_df(int argc, char* argv[]);
void cmd_tree(int argc, char* argv[]);
void cmd_find(int argc, char* argv[]);
void cmd_wifi(int argc, char* argv[]);
void cmd_ping(int argc, char* argv[]);
void cmd_httpget(int argc, char* argv[]);
void cmd_reboot(int argc, char* argv[]);
void cmd_neofetch(int argc, char* argv[]);
void cmd_minic(int argc, char* argv[]);
void cmd_nano(int argc, char* argv[]);
void cmd_about(int argc, char* argv[]);
