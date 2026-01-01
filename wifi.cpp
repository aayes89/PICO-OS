#include "wifi.h"
#include <LittleFS.h>

void loadWiFiConfig() {
  File f = LittleFS.open("/etc/wifi.conf", "r");
  if (f) {
    String line = f.readStringUntil('\n');
    int sep = line.indexOf(':');
    if (sep > 0) {
      String ssid = line.substring(0, sep);
      String pass = line.substring(sep + 1);
      WiFi.begin(ssid.c_str(), pass.c_str());
      // Espera conexión...
    }
    f.close();
  }
}
const char* encToString(uint8_t enc) {
  switch (enc) {
    case ENC_TYPE_NONE: return "NONE";
    case ENC_TYPE_TKIP: return "WPA";
    case ENC_TYPE_CCMP: return "WPA2";
    case ENC_TYPE_AUTO: return "AUTO";
    default:
      return "UNK";
      /*case 0:
      return "OPEN";  // CYW43_AUTH_OPEN
    //case CYW43_AUTH_WEP: return "WEP";          // WEP not implemented
    case 0x00200002: return "WPA_TKIP_PSK";       // CYW43_AUTH_WPA_TKIP_PSK
    case 0x00400004: return "WPA2_AES_PSK";       // CYW43_AUTH_WPA2_AES_PSK
    case 0x00400006: return "WPA2_MIXED_PSK";     // CYW43_AUTH_WPA2_MIXED_PSK
    case 0x01000004: return "WPA3_SAE_AES_PSK";   // CYW43_AUTH_WPA3_SAE_AES_PSK
    case 0x01400004: return "WPA3_WPA2_AES_PSK";  // CYW43_AUTH_WPA3_WPA2_AES_PSK
    */
  }
}
const char* macToString(uint8_t mac[6]) {
  static char s[20];
  sprintf(s, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return s;
}