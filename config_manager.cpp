#include "config_manager.h"

DeviceConfig config;
const char* configPath = "/config.bin";
const KeyMap hidKeys[] = {
  {0, "Keine"}, 
  {97, "a"}, {98, "b"}, {99, "c"}, {100, "d"}, {101, "e"}, {102, "f"}, {103, "g"}, {104, "h"}, {105, "i"}, {106, "j"}, {107, "k"}, {108, "l"}, {109, "m"}, {110, "n"}, {111, "o"}, {112, "p"}, {113, "q"}, {114, "r"}, {115, "s"}, {116, "t"}, {117, "u"}, {118, "v"}, {119, "w"}, {120, "x"}, {121, "y"}, {122, "z"},
  {48, "0"}, {49, "1"}, {50, "2"}, {51, "3"}, {52, "4"}, {53, "5"}, {54, "6"}, {55, "7"}, {56, "8"}, {57, "9"},
  {176, "Enter"}, {177, "Esc"}, {178, "Backsp."}, {179, "Tab"}, {32, "Space"},
  {194, "F1"}, {195, "F2"}, {196, "F3"}, {197, "F4"}, {198, "F5"}, {199, "F6"}, {200, "F7"}, {201, "F8"}, {202, "F9"}, {203, "F10"}, {204, "F11"}, {205, "F12"},
  {209, "Einfg"}, {212, "Entf"}, {210, "Pos1"}, {213, "Ende"}, {211, "BildAuf"}, {214, "BildAb"},
  {215, "Rechts"}, {216, "Links"}, {217, "Unten"}, {218, "Oben"},
  
  // Media Keys (Interne IDs für unser Switch-Case)
  {401, "Vol +"}, {402, "Vol -"}, {403, "Mute"}, {404, "Play/Pause"}, {405, "Next"}, {406, "Prev"},
  
  // Maus-Sektion
  {3001, "Maus: Scroll Up"}, {3002, "Maus: Scroll Down"}, {3003, "Maus: Klick Links"}, {3004, "Maus: Klick Rechts"}
};
const int keyCount = sizeof(hidKeys) / sizeof(KeyMap);

void saveConfig() {
  File f = LittleFS.open(configPath, "w");
  if (f) {
    f.write((uint8_t*)&config, sizeof(DeviceConfig));
    f.close();
  }
}

bool loadConfig() {
  strncpy(config.deviceName, "HID-Panel", 32);
  if (LittleFS.exists(configPath)) {
    File f = LittleFS.open(configPath, "r");
    if (f) {
      f.read((uint8_t*)&config, sizeof(DeviceConfig));
      f.close();
      return true;
    }
  }
  
  // Default Werte setzen, wenn keine Datei existiert
  for (int i = 0; i < 8; i++) {
    snprintf(config.buttons[i].label, 9, "B%d", i + 1);
    config.buttons[i].pinInput = -1;
    config.buttons[i].pinLED = -1;
    config.buttons[i].isSwitch = 0;
  }
  // Optional: Initiale leere Szenen
  for (int i = 0; i < 5; i++) {
    snprintf(config.scenes[i].name, 17, "Szene %d", i + 1);
    config.scenes[i].active = (i == 0); // Nur erste Szene aktiv
  }
  return false;
}
