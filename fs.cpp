#include "fs.h"
#include "io.h"

bool fsDirty = false;
FSInfo fs_info;

void initFS() {
  Serial.print("Intentando montar LittleFS... ");
  if (LittleFS.begin()) {
    Serial.println("OK - montado correctamente");
    // Info de espacio (compatible con tu versión, usando info())
    if (LittleFS.info(fs_info)) {
      Serial.printf("Total: %u bytes | Usado: %u bytes | Libre aprox: %u bytes\n",
                    fs_info.totalBytes,
                    fs_info.usedBytes,
                    fs_info.totalBytes - fs_info.usedBytes);
      // Crear estructura Linux-like si no existe
      if (!LittleFS.exists("/bin")) LittleFS.mkdir("/bin");
      if (!LittleFS.exists("/home")) LittleFS.mkdir("/home");
      if (!LittleFS.exists("/etc")) LittleFS.mkdir("/etc");
      if (!LittleFS.exists("/sys")) LittleFS.mkdir("/sys");
      // Crear /etc/passwd vacío (formato user:pass:permisos)
      if (!LittleFS.exists("/etc/passwd")) {
        File f = LittleFS.open("/etc/passwd", "w");
        f.close();
      }
      // Crear /etc/wifi.conf vacío
      if (!LittleFS.exists("/etc/wifi.conf")) {
        File f = LittleFS.open("/etc/wifi.conf", "w");
        f.close();
      }
      // Crear /etc/network.conf vacío por si W5500
      if (!LittleFS.exists("/etc/network.conf")) {
        File f = LittleFS.open("/etc/network.conf", "w");
        f.close();
      }
    } else {
      Serial.println("No se pudo obtener info del filesystem");
    }
  } else {
    Serial.println("FALLO al montar → intentando formatear...");
    if (LittleFS.format()) {
      Serial.println("Formateado OK, volviendo a montar...");
      if (LittleFS.begin()) {
        Serial.println("Montado tras format OK");
      } else {
        Serial.println("AÚN FALLA después de format → problema grave");
      }
    } else {
      Serial.println("FALLO TOTAL al formatear");
    }
  }
}
void markDirty() {
  fsDirty = true;
}
void flushFS() {
  if (fsDirty) fsDirty = false;
}
bool makeDirSafe(const String& p) {
#if defined(ARDUINO_ARCH_RP2040)
  return LittleFS.mkdir(p);
#else
  // Simular directorio creando marcador
  String marker = p + "/.dir";
  File f = LittleFS.open(marker, "w");
  if (!f) return false;
  f.close();
  return true;
#endif
}
String normalizePath(const char* path) {
  if (!path || !*path) return currentPath;
  String p = path[0] == '/' ? path : (currentPath + "/" + path);
  // Normalización básica . y ..
  while (p.indexOf("//") >= 0) p.replace("//", "/");
  if (p.length() > 1 && p.endsWith("/")) p.remove(p.length() - 1);
  // Resolver .. manualmente
  while (true) {
    int pos = p.indexOf("/..");
    if (pos < 0) break;
    int prev = p.lastIndexOf('/', pos - 1);
    p.remove(prev >= 0 ? prev : 0, (pos + 3) - (prev >= 0 ? prev : 0));
    if (p.length() == 0) p = "/";
  }
  if (p.length() == 0 || p[0] != '/') p = "/" + p;
  if (p == "") p = "/";
  return p;
}
bool isDir(const String& path) {
  File f = LittleFS.open(path, "r");
  if (!f) return false;
  bool d = f.isDirectory();
  f.close();
  return d;
}
bool existsFileOrDir(const String& path) {
  return LittleFS.exists(path);
}
String basenameOf(const String& p) {
  int i = p.lastIndexOf('/');
  return (i < 0) ? p : p.substring(i + 1);
}
String parentOf(const String& p) {
  int i = p.lastIndexOf('/');
  if (i <= 0) return "/";
  return p.substring(0, i);
}
bool removeRecursive(const String& path) {
  File f = LittleFS.open(path, "r");
  if (!f) return false;
  if (f.isDirectory()) {
    File c = f.openNextFile();
    while (c) {
      String child = String(c.name());
      c.close();
      if (!removeRecursive(child)) return false;
      c = f.openNextFile();
    }
    f.close();
    return LittleFS.rmdir(path);
  }
  f.close();
  return LittleFS.remove(path);
}
bool copyRecursive(const String& srcPath, const String& dstPath) {
  if (!LittleFS.mkdir(dstPath)) {
    if (!existsFileOrDir(dstPath)) return false;  // si no es dir y no se pudo crear
  }
  File dir = LittleFS.open(srcPath, "r");
  if (!dir || !dir.isDirectory()) return false;
  File entry = dir.openNextFile();
  while (entry) {
    String name = entry.name();
    String srcFull = srcPath + "/" + name;
    String dstFull = dstPath + "/" + name;
    if (entry.isDirectory()) {
      if (!copyRecursive(srcFull, dstFull)) {
        entry.close();
        return false;
      }
    } else {
      File s = LittleFS.open(srcFull, "r");
      File d = LittleFS.open(dstFull, "w");
      if (!s || !d) {
        if (s) s.close();
        if (d) d.close();
        entry.close();
        return false;
      }
      uint8_t buf[256];
      int n;
      while ((n = s.read(buf, sizeof(buf))) > 0) {
        d.write(buf, n);
      }
      s.close();
      d.close();
    }
    entry = dir.openNextFile();
  }
  entry.close();
  dir.close();
  return true;
}
void appendToFile(const String& path, const String& text) {
  File f = LittleFS.open(path, "a");  // modo append
  if (!f) {
    outPrintln("Error abriendo archivo para append");
    return;
  }
  f.println(text);
  f.close();
  outPrintln("Texto añadido al archivo");
}
void treeRecursive(const String& path, String prefix) {
  File dir = LittleFS.open(path, "r");
  if (!dir || !dir.isDirectory()) return;
  File entry = dir.openNextFile();
  int count = 0;
  while (entry) {
    count++;
    String name = entry.name();
    bool last = !dir.openNextFile();  // peek next para saber si es último
    dir.openNextFile();               // restore position (hacky pero funciona)
    outPrint(prefix);
    if (last) {
      outPrint("└── ");
    } else {
      outPrint("├── ");
    }
    outPrint(name);
    if (entry.isDirectory()) outPrint("/");
    outPrintln();
    if (entry.isDirectory()) {
      String newPrefix = prefix + (last ? "    " : "│   ");
      treeRecursive(path + "/" + name, newPrefix);
    }
    entry = dir.openNextFile();
  }
  dir.close();
}
void findRecursive(const String& path, const String& target) {
  File dir = LittleFS.open(path, "r");
  if (!dir || !dir.isDirectory()) return;
  File entry = dir.openNextFile();
  while (entry) {
    String name = entry.name();
    if (name.indexOf(target) >= 0) {
      outPrintln(path + "/" + name + (entry.isDirectory() ? "/" : ""));
    }
    if (entry.isDirectory()) {
      findRecursive(path + "/" + name, target);
    }
    entry = dir.openNextFile();
  }
  dir.close();
}