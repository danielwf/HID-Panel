#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <LittleFS.h>

// --- STRUKTUREN ---
struct KeyMap { 
  uint16_t code; 
  const char* name; 
};

struct ButtonConfig { 
  char label[9]; 
  int pinInput; 
  int pinLED; 
  int isSwitch; // 0 = Taster, 1 = Schalter
};

struct SceneConfig { 
  char name[17]; 
  bool active; 
  uint16_t btnK[8];    // Key bei Tastendruck / Schalter AN
  uint8_t btnM[8];     // Modifier dazu
  uint16_t btnOffK[8]; // Key bei Schalter AUS
  uint8_t btnOffM[8];  // Modifier dazu
  uint16_t encK[4];    // Encoder: L, R, L-Fast, R-Fast
  uint8_t encM[4]; 
};

struct DeviceConfig { 
  int encA = 32; 
  int encB = 12; 
  int stepsPerRev = 600;   
  int stepsSlow = 20;      
  int speedThreshold = 360; 
  char deviceName[32];
  ButtonConfig buttons[8]; 
  SceneConfig scenes[5]; 
};

extern DeviceConfig config;
extern const char* configPath;
extern const KeyMap hidKeys[];
extern const int keyCount;

// --- FUNKTIONEN ---
void saveConfig();
bool loadConfig(); 

#endif