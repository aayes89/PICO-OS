#include "commands.h"
#include "io.h"
#include "fs.h"
#include "wifi.h"
#include "engine/mini_c.c"
#include "editor.h"

extern unsigned long startTime;  // Para calcular uptime

Command commands[] = {
  { "help", cmd_help, "Muestra esta ayuda" },
  { "echo", cmd_echo, "Repite los argumentos" },
  { "clear", cmd_clear, "Limpia la pantalla" },
  { "uptime", cmd_uptime, "Muestra tiempo transcurrido" },
  { "led", cmd_led, "led on|off - controla LED" },
  { "ls", cmd_ls, "Lista archivos y dirs" },
  { "mkdir", cmd_mkdir, "Crea directorio" },
  { "cd", cmd_cd, "Cambia directorio" },
  { "cp", cmd_cp, "cp origen destino - copia archivo" },
  { "mv", cmd_mv, "mv origen destino - mueve o renombra" },
  { "pwd", cmd_pwd, "Muestra directorio actual" },
  { "touch", cmd_touch, "Crea archivo vacío" },
  { "rm", cmd_rm, "Borra archivo o dir" },
  { "cat", cmd_cat, "Muestra contenido de archivo" },
  { "write", cmd_write, "write \"text\" file - escribe en archivo (sobrescribe)" },
  { "df", cmd_df, "Muestra uso del filesystem" },
  { "tree", cmd_tree, "Muestra estructura de directorios" },
  { "find", cmd_find, "Busca archivos por nombre (parcial)" },
  { "wifi", cmd_wifi, "Gestión WiFi: status, scan, connect, ip, disconnect, ap" },
  { "ping", cmd_ping, "Ping a IP o dominio" },
  { "httpget", cmd_httpget, "GET HTTP simple a URL" },
  { "reboot", cmd_reboot, "Reinicia el Pico" },
  { "neofetch", cmd_neofetch, "Muestra info del sistema con estilo neofetch" },
  { "minic", cmd_minic, "Intérprete minimalista para lenguaje C" },
  { "nano", cmd_nano, "Editor de texto estilo nano" },
  { "about", cmd_about, "Acerca del sistema." },
  { nullptr, nullptr, nullptr }  // fin de lista
};

// =============================================
//  Implementación de los comandos
// =============================================
void cmd_help(int argc, char* argv[]) {
  outPrintln("Comandos disponibles:");
  for (int i = 0; commands[i].name != nullptr; i++) {
    outPrint("  ");
    outPrint(commands[i].name);
    outPrint("\t- ");
    outPrintln(commands[i].help);
  }
}
void cmd_clear(int argc, char* argv[]) {
  for (int i = 0; i < 40; i++) outPrintln();
  outPrintln("(pantalla \"limpia\")");
}
void cmd_uptime(int argc, char* argv[]) {
  unsigned long secs = (millis() - startTime) / 1000;
  outPrint("Uptime: ");
  outPrint(String(secs / 3600));
  outPrint("h ");
  outPrint(String((secs % 3600) / 60));
  outPrint("m ");
  outPrint(String(secs % 60));
  outPrintln("s");
}
void cmd_led(int argc, char* argv[]) {
  if (argc < 2) {
    outPrintln("Uso: led on | led off");
    return;
  }
  if (strcmp(argv[1], "on") == 0) {
    digitalWrite(LED_PIN, HIGH);
    outPrintln("LED encendido");
  } else if (strcmp(argv[1], "off") == 0) {
    digitalWrite(LED_PIN, LOW);
    outPrintln("LED apagado");
  } else {
    outPrintln("Argumento inválido. Usa on o off");
  }
}
void cmd_ls(int argc, char* argv[]) {
  String path = (argc > 1) ? normalizePath(argv[1]) : currentPath;
  File dir = LittleFS.open(path, "r");
  if (!dir || !dir.isDirectory()) {
    outPrintln("No es un directorio");
    return;
  }
  File f = dir.openNextFile();
  while (f) {
    outPrint(f.name());
    if (f.isDirectory()) outPrint("/");
    outPrint("\t");
    outPrintln(String(f.size()));
    f = dir.openNextFile();
  }
}
void cmd_mkdir(int argc, char* argv[]) {
  if (argc < 2) {
    outPrintln("Uso: mkdir dir_name");
    return;
  }
  String path = normalizePath(argv[1]);
  if (existsFileOrDir(path)) {
    outPrintln("Ya existe");
    return;
  }
  if (!makeDirSafe(path)) {
    outPrintln("Error creando dir");
    return;
  }
  markDirty();
}
void cmd_cd(int argc, char* argv[]) {
  if (argc < 2) {
    currentPath = "/";
    return;
  }
  String np = normalizePath(argv[1]);
  if (!isDir(np)) {
    outPrintln("No es un directorio");
    return;
  }
  currentPath = np;
}
void cmd_pwd(int argc, char* argv[]) {
  outPrintln(currentPath);
}
void cmd_touch(int argc, char* argv[]) {
  if (argc < 2) {
    outPrintln("Uso: touch file");
    return;
  }
  String path = normalizePath(argv[1]);
  if (existsFileOrDir(path)) {
    outPrintln("Ya existe");
    return;
  }
  File f = LittleFS.open(path, "w");
  if (!f) {
    outPrintln("Error creando archivo");
    return;
  }
  f.close();
  markDirty();
}
void cmd_rm(int argc, char* argv[]) {
  if (argc < 2) {
    outPrintln("Uso: rm path");
    outPrintln("     rm -r path  (borrar directorio recursivamente)");
    return;
  }
  bool recursive = false;
  const char* pathArg = argv[1];
  if (argc == 3 && strcmp(argv[1], "-r") == 0) {
    recursive = true;
    pathArg = argv[2];
  }
  String path = normalizePath(pathArg);
  if (!existsFileOrDir(path)) {
    outPrintln("No encontrado");
    return;
  }
  if (recursive) {
    if (!removeRecursive(path)) {
      outPrintln("Error eliminando recursivamente");
      return;
    }
  } else {
    // Solo archivo o dir vacío
    if (isDir(path)) {
      outPrintln("Es un directorio. Usa 'rm -r' para eliminarlo");
      return;
    }
    if (!LittleFS.remove(path)) {
      outPrintln("Error eliminando archivo");
      return;
    }
  }
  outPrintln("OK");
}
void cmd_cat(int argc, char* argv[]) {
  if (argc < 2) {
    outPrintln("Uso: cat file");
    return;
  }
  String path = normalizePath(argv[1]);
  File f = LittleFS.open(path, "r");
  if (!f || f.isDirectory()) {
    outPrintln("No es un archivo");
    return;
  }
  uint8_t buf[256];
  int n;
  while ((n = f.read(buf, sizeof(buf))) > 0) {
    if (isWebCommand) {
      outputBuffer += String((char*)buf).substring(0, n);
    } else {
      Serial.write(buf, n);
    }
  }
  outPrintln();
  f.close();
}
void cmd_write(int argc, char* argv[]) {
  if (argc < 3) {
    outPrintln("Uso: write \"text\" file");
    return;
  }
  String path = normalizePath(argv[2]);
  File f = LittleFS.open(path, "w");
  if (!f) {
    outPrintln("Error abriendo archivo");
    return;
  }
  f.print(argv[1]);
  f.close();
  markDirty();
}
void cmd_cp(int argc, char* argv[]) {
  if (argc < 3) {
    outPrintln("Uso: cp origen destino");
    outPrintln("     cp -r origen destino  (para copiar directorio recursivamente)");
    return;
  }
  bool recursive = false;
  const char* srcArg = argv[1];
  const char* dstArg = argv[2];
  // Detectar opción -r
  if (argc == 4 && strcmp(argv[1], "-r") == 0) {
    recursive = true;
    srcArg = argv[2];
    dstArg = argv[3];
  } else if (argc > 4) {
    outPrintln("Demasiados argumentos");
    return;
  }
  String src = normalizePath(srcArg);
  String dst = normalizePath(dstArg);
  if (!existsFileOrDir(src)) {
    outPrintln("Origen no encontrado");
    return;
  }
  // Caso archivo simple
  if (!isDir(src)) {
    File s = LittleFS.open(src, "r");
    if (!s) {
      outPrintln("Error abriendo origen");
      return;
    }
    File d = LittleFS.open(dst, "w");
    if (!d) {
      outPrintln("Error creando destino");
      s.close();
      return;
    }
    uint8_t buf[256];
    int n;
    while ((n = s.read(buf, sizeof(buf))) > 0) {
      d.write(buf, n);
    }
    s.close();
    d.close();
    outPrintln("Archivo copiado");
    return;
  }
  // Caso directorio
  if (!recursive) {
    outPrintln("Origen es directorio. Usa 'cp -r' para copiar recursivamente");
    return;
  }
  // Copia recursiva
  if (!copyRecursive(src, dst)) {
    outPrintln("Error durante copia recursiva");
    return;
  }
  outPrintln("Directorio copiado recursivamente");
}
void cmd_mv(int argc, char* argv[]) {
  if (argc < 3) {
    outPrintln("Uso: mv origen destino");
    return;
  }
  String src = normalizePath(argv[1]);
  String dst = normalizePath(argv[2]);
  if (!existsFileOrDir(src)) {
    outPrintln("Origen no encontrado");
    return;
  }
  if (!LittleFS.rename(src, dst)) {
    outPrintln("Error moviendo/renombrando");
    return;
  }
  markDirty();
  outPrintln("OK");
}
void cmd_df(int argc, char* argv[]) {
  if (LittleFS.begin()) {
    if (LittleFS.info(fs_info)) {
      outPrintf("Total: %u bytes\nUsado: %u bytes\nLibre aprox: %u bytes\nBloque: %u bytes | Página: %u bytes\n",
                fs_info.totalBytes,
                fs_info.usedBytes,
                fs_info.totalBytes - fs_info.usedBytes,
                fs_info.blockSize,
                fs_info.pageSize);
    } else {
      outPrintln("No se pudo obtener info del filesystem");
    }
  }
}
void cmd_echo(int argc, char* argv[]) {
  if (argc < 2) {
    outPrintln("Uso:");
    outPrintln("  echo texto           → muestra texto");
    outPrintln("  echo \"texto\" >> archivo  → añade al final del archivo");
    return;
  }
  // Caso simple: solo mostrar
  bool append = false;
  String filePath;
  // Buscar si hay ">>"
  for (int i = 1; i < argc - 1; i++) {
    if (strcmp(argv[i], ">>") == 0) {
      append = true;
      if (i + 1 >= argc) {
        outPrintln("Falta nombre de archivo después de >>");
        return;
      }
      filePath = normalizePath(argv[i + 1]);
      // Reconstruir texto a añadir
      String text;
      for (int j = 1; j < i; j++) {
        text += argv[j];
        if (j < i - 1) text += " ";
      }
      appendToFile(filePath, text);
      return;
    }
  }
  // Si no hay >>, mostrar normal
  for (int i = 1; i < argc; i++) {
    outPrint(argv[i]);
    if (i < argc - 1) outPrint(" ");
  }
  outPrintln();
}
void cmd_tree(int argc, char* argv[]) {
  String path = (argc > 1) ? normalizePath(argv[1]) : "/";
  outPrintln(path);
  treeRecursive(path, "  ");
}
void cmd_find(int argc, char* argv[]) {
  if (argc < 2) {
    outPrintln("Uso: find nombre");
    return;
  }
  String nameToFind = argv[1];
  outPrintln("Buscando: " + nameToFind);
  findRecursive("/", nameToFind);
}
void cmd_wifi(int argc, char* argv[]) {
  if (argc < 2) {
    outPrintln("Uso: wifi status | scan | connect SSID PASS [seguridad] | ip | disconnect | ap SSID PASS");
    outPrintln("Seguridad opcional: open, wpa, wpa2 (default wpa2)");
    return;
  }
  String subcmd = argv[1];
  if (subcmd == "status") {
    if (WiFi.status() == WL_CONNECTED) {
      outPrint("Conectado a: ");
      outPrintln(WiFi.SSID());
      outPrint("IP: ");
      outPrintln(WiFi.localIP().toString());
      uint8_t bssid[6];
      WiFi.BSSID(0, bssid);
      outPrintf("MAC:        %s\n", macToString(bssid));
      outPrintf("RSSI:       %d dBm\n", WiFi.RSSI());
      outPrintf("Canal:      %d\n", WiFi.channel());
    } else if (WiFi.status() == WL_IDLE_STATUS) {
      outPrintln("Estado: IDLE");
    } else if (WiFi.status() == WL_DISCONNECTED) {
      outPrintln("Estado: Desconectado");
    } else {
      outPrintln("No conectado a ninguna red");
      outPrintf("Estado: %d\n", WiFi.status());
    }
  } else if (subcmd == "scan") {
    //WiFi.mode(CYW43_ITF_STA); // WIFI_STA
    WiFi.disconnect();
    delay(200);
    outPrintf("Escaneando redes...\n");
    auto nets = WiFi.scanNetworks();
    if (nets <= 0) {
      outPrintf("No se encontraron redes\n");
    } else {
      outPrintf("Found %d networks\n", nets);
      outPrintf("%32s %5s %17s %2s %4s\n", "SSID", "ENC", "BSSID         ", "CH", "RSSI");
      for (auto i = 0; i < nets; i++) {
        uint8_t bssid[6];
        WiFi.BSSID(i, bssid);
        outPrintf("%32s %5s %17s %2d %4ld\n", WiFi.SSID(i), encToString(WiFi.encryptionType(i)), macToString(bssid), WiFi.channel(i), WiFi.RSSI(i));
      }
    }
    WiFi.scanDelete();
  } else if (subcmd == "connect" && argc >= 4) {
    const char* ssid = argv[2];
    const char* pass = argv[3];
    outPrintf("Conectando a %s...\n", ssid);
    //WiFi.mode(CYW43_ITF_STA); // WIFI_STA
    WiFi.disconnect();
    delay(200);
    WiFi.begin(ssid, pass);
    int attempts = 0;
    // 20 intentos
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      outPrint(".");
      attempts++;
    }
    outPrintln();
    if (WiFi.status() == WL_CONNECTED) {
      outPrintf("Conectado!\n");
      outPrint("IP: ");
      outPrintln(WiFi.localIP().toString());
    } else {
      outPrintf("Fallo al conectar (timeout o credencial incorrectos)\n");
    }
  } else if (subcmd == "ip") {
    if (WiFi.status() == WL_CONNECTED) {
      outPrint("IP: ");
      outPrintln(WiFi.localIP().toString());
    } else {
      outPrintf("No conectado\n");
    }
  } else if (subcmd == "disconnect") {
    WiFi.disconnect(true);
    outPrintf("WiFi desconectado\n");
  } else if (subcmd == "ap" && argc >= 4) {
    const char* ssid = argv[2];
    const char* pass = argv[3];
    outPrintf("Iniciando AP '%s' ...\n", ssid);
    //WiFi.mode(CYW43_ITF_AP); // WIFI_AP
    bool ok = WiFi.softAP(ssid, pass);
    if (ok) {
      outPrint("AP iniciado - IP: ");
      outPrintln(WiFi.softAPIP().toString());
    } else {
      outPrintf("Fallo al iniciar AP\n");
    }
  } else {
    outPrintf("Subcomando desconocido\n");
  }
}
void cmd_ping(int argc, char* argv[]) {
  if (argc < 2) {
    outPrintf("Uso: ping IP_o_Dominio [veces]\n");
    return;
  }
  String target = argv[1];
  int count = (argc > 2) ? atoi(argv[2]) : 4;
  outPrintf("PING %s (%s) - %d paquetes:\n", target.c_str(), target.c_str(), count);
  for (int i = 0; i < count; i++) {
    unsigned long time = WiFi.ping(target.c_str());
    if (time != ~0UL) {
      outPrintf("  %d) %lu ms\n", i + 1, time);
    } else {
      outPrintf("  %d) Timeout\n", i + 1);
    }
    delay(1000);
  }
}
void cmd_httpget(int argc, char* argv[]) {
  if (argc < 2) {
    outPrintln("Uso: httpget URL");
    return;
  }
  String url = argv[1];
  if (!url.startsWith("http")) {
    outPrintln("URL debe empezar con http:// o https://");
    return;
  }
  WiFiClient client;
  HTTPClient http;
  outPrintf("GET %s ... ", url.c_str());
  http.begin(client, url);
  int httpCode = http.GET();
  if (httpCode > 0) {
    outPrintf("OK - código %d\n", httpCode);
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
      String payload = http.getString();
      outPrintln("Respuesta (primeros 500 chars):");
      outPrintln(payload.substring(0, 500));
      if (payload.length() > 500) outPrintln("... (truncado)");
    }
  } else {
    outPrintf("Error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}
void cmd_about(int argc, char* argv[]) {
  outPrintln("PICO-OS V1");
  outPrintln("BusyBox-like + Filesystem con LittleFS + WiFI + WebServer");
  outPrintln("Hecho por Slam 12/2025");
}
void cmd_neofetch(int argc, char* argv[]) {
  // ASCII Art:
  const char* ascii[] = {
    "          /\\",
    "         /  \\",
    "        / /\\ \\",
    "       / /  \\ \\",
    "      / /____\\ \\",
    "     /_/______\\_\\",
    "        /____\\",
    "         /  \\",
    "        /____\\",
    "          ||",
    "          ||",
    "\x1b[32m        Raspberry\x1b[0m \x1b[36mPi\x1b[0m \x1b[33mPico W\x1b[0m"
  };
  const int ascii_lines = sizeof(ascii) / sizeof(ascii[0]);
  // Obtener datos del sistema
  unsigned long uptime_secs = (millis() - startTime) / 1000;
  unsigned long uptime_h = uptime_secs / 3600;
  unsigned long uptime_m = (uptime_secs % 3600) / 60;
  unsigned long uptime_s = uptime_secs % 60;
  FSInfo fs_info;
  LittleFS.info(fs_info);
  uint32_t fs_total = fs_info.totalBytes;
  uint32_t fs_used = fs_info.usedBytes;
  String wifi_status = (WiFi.status() == WL_CONNECTED)
                         ? "Conectado (" + WiFi.SSID() + ")"
                         : "Desconectado";
  float temp = 25.0;  // Placeholder (puedes implementar sensor real si quieres)
  // Imprimir línea por línea
  for (int i = 0; i < ascii_lines; i++) {
    // ASCII art a la izquierda
    outPrint(ascii[i]);
    // Espacio separador
    outPrint("     ");
    // Información a la derecha (solo en líneas seleccionadas)
    switch (i) {
      case 0:
        outPrint("\033[1;32mPICO-OS\033[0m");
        break;
      case 2:
        outPrint("-----------------------------");
        break;
      case 4:
        outPrintf("Uptime:     %lu h %lu m %lu s", uptime_h, uptime_m, uptime_s);
        break;
      case 6:
        outPrintf("Free RAM:   %u / %u bytes", rp2040.getFreeHeap(), rp2040.getTotalHeap());
        break;
      case 8:
        outPrintf("LittleFS:   %u / %u bytes usados", fs_used, fs_total);
        break;
      case 10:
        outPrintf("WiFi:       %s", wifi_status.c_str());
        break;
      case 12:
        outPrintf("Temp (aprox): %.1f °C", temp);
        break;
      default:
        outPrint(" ");
        break;
    }
    outPrintln();
  }
  outPrintln();
}
void cmd_minic(int argc, char** argv) {
  if (argc < 2) {
    outPrintln("Uso:");
    outPrintln("  minic \"código aquí\"                  → ejecuta código directamente");
    outPrintln("  minic file nombre_archivo.mini         → ejecuta desde archivo en LittleFS");
    outPrintln("  minic help                             → muestra esta ayuda");
    return;
  }

  // Ayuda explícita
  if (strcmp(argv[1], "help") == 0) {
    cmd_minic(1, NULL);  // recursión ligera solo para mostrar ayuda
    return;
  }

  // -------------------------------------------------------
  // Modo 1: Código directo entre comillas
  // -------------------------------------------------------
  if (argc == 2 && (argv[1][0] == '"' || argv[1][0] == '\'')) {
    // Quitamos las comillas externas si existen
    char* code = argv[1];
    if (code[0] == '"' || code[0] == '\'') {
      code++;
      size_t len = strlen(code);
      if (len > 0 && (code[len - 1] == '"' || code[len - 1] == '\'')) {
        code[len - 1] = '\0';
      }
    }

    outPrintln("Ejecutando código directo...\n");
    outPrint("Código: ");
    outPrintln(code);
    outPrintln("----------------------------------------");

    minic_run(code);

    outPrintln("----------------------------------------");
    outPrintln("[MiniC finalizado]");
    return;
  }

  // -------------------------------------------------------
  // Modo 2: Cargar desde archivo en LittleFS
  // -------------------------------------------------------
  if (argc >= 3 && strcmp(argv[1], "file") == 0) {
    const char* filename = argv[2];

    // Intentamos abrir el archivo
    File file = LittleFS.open(filename, "r");
    if (!file) {
      outPrint("Error: No se pudo abrir el archivo: ");
      outPrintln(filename);
      return;
    }

    // Calculamos tamaño
    size_t size = file.size();
    if (size == 0) {
      outPrintln("El archivo está vacío");
      file.close();
      return;
    }
    if (size > 4096) {  // límite razonable para evitar desbordamientos
      outPrintln("Error: Programa demasiado grande (>4KB)");
      file.close();
      return;
    }

    // Reservamos buffer
    char* buffer = (char*)malloc(size + 1);
    if (!buffer) {
      outPrintln("Error: No hay suficiente memoria");
      file.close();
      return;
    }

    // Leemos todo el contenido
    size_t read = file.readBytes(buffer, size);
    buffer[read] = '\0';  // terminador nulo importante!!
    file.close();

    outPrint("Ejecutando desde archivo: ");
    outPrintln(filename);
    outPrint("Tamaño: ");
    outPrint(String(read));
    outPrintln(" bytes");
    outPrintln("----------------------------------------");

    minic_run(buffer);

    outPrintln("----------------------------------------");
    outPrintln("[MiniC finalizado]");

    free(buffer);
    return;
  }

  // Si no entendimos el formato
  outPrintln("Formato no reconocido.");
  outPrintln("Usa: minic help  para ver las opciones.");
}
//static NanoLite editor;
static Editor editor;

void cmd_nano(int argc, char* argv[]) {
  if (argc < 2) {
    outPrintln("Uso: nano archivo");
    return;
  }

  editor.openFile(argv[1]);
  editor.run();
}

void cmd_reboot(int argc, char* argv[]) {
  outPrintln("Reiniciando Pico en 2 segundos...");
  delay(2000);
  rp2040.restart();
}