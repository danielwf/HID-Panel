#ifndef HARDWARE_IO_H
#define HARDWARE_IO_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include "config_manager.h"

// Globale Hardware-Objekte
extern Adafruit_SSD1306 display;
extern bool hasDisplay;
extern bool displayActive;
extern unsigned long displayTimeout;
extern int currentSceneIdx;

// Encoder-Variablen (volatile für ISR)
extern volatile long encoderTicks;

// Funktionen
void initHardware();      // Pins, Wire, Display initialisieren
void updateHardware();    // Loop für Buttons und Display-Timeout
void updateDisplay();     // Szenenname zeichnen
void startBlink(int idx); // LED-Feedback
void checkRebootCombo();  // Die Taste 1+2 Logik

// ISR (muss im RAM liegen)
void readEncoderISR();

#endif
