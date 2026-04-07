#include "hardware_io.h"
#include "wifi_portal.h"
#include "config_manager.h"
volatile long encoderTicks = 0;
volatile unsigned long lastISRTime = 0;

// Globale Instanzen (extern in hardware_io.h deklariert)
Adafruit_SSD1306 display(128, 64, &Wire, -1);
bool hasDisplay = false;
bool displayActive = true;
unsigned long displayTimeout = 0;
int currentSceneIdx = 0;

// Encoder-Variablen (volatile, da sie in der ISR geändert werden)
volatile long deltaSlow = 0;
volatile long deltaFast = 0;

// Struktur für LED-Blink-Tasks
struct BlinkTask { 
    int pin; 
    int count; 
    unsigned long nextToggle; 
    bool state; 
} blinkers[8];

// --- INTERRUPT SERVICE ROUTINE (ISR) ---
// Optimiert für 600 PPR Inkrementalgeber

void IRAM_ATTR readEncoderISR() {
    int curA = digitalRead(config.encA);
    int dir = (digitalRead(config.encB) != curA) ? 1 : -1;
    encoderTicks += dir;
    lastISRTime = millis();
}

// --- INITIALISIERUNG ---
void initHardware() {
    // 1. I2C Bus & Display
    Wire.begin(21, 22);
    if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        hasDisplay = true;
        display.clearDisplay();
        display.display();
    }

    // 2. Encoder Setup (600 PPR braucht stabile Pullups)
    if (config.encA != -1 && config.encB != -1) {
        pinMode(config.encA, INPUT_PULLUP);
        pinMode(config.encB, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(config.encA), readEncoderISR, CHANGE);
    }

    // 3. Buttons & LEDs
    for (int i = 0; i < 8; i++) {
        if (config.buttons[i].pinInput != -1) {
            pinMode(config.buttons[i].pinInput, INPUT_PULLUP);
        }
        if (config.buttons[i].pinLED != -1) {
            pinMode(config.buttons[i].pinLED, OUTPUT);
            digitalWrite(config.buttons[i].pinLED, LOW);
            blinkers[i].pin = config.buttons[i].pinLED;
            blinkers[i].count = 0;
        }
    }
    displayTimeout = millis();
}

// --- DISPLAY-MANAGEMENT ---
void updateDisplay() {
    if (!hasDisplay || configModeActive) return; 
    
    display.clearDisplay();
    if (displayActive) {
        display.setTextColor(WHITE);
        display.setTextSize(3);
        display.setCursor(10, 20);
        
        if (strlen(config.scenes[currentSceneIdx].name) > 0) {
            display.print(config.scenes[currentSceneIdx].name);
        } else {
            display.printf("S %d", currentSceneIdx + 1);
        }
    }
    display.display();
}

// --- FEEDBACK & REBOOT ---
void startBlink(int idx) {
    if (idx < 0 || idx >= 8) return;
    if (config.buttons[idx].pinLED != -1) {
        blinkers[idx].count = 6; 
        blinkers[idx].nextToggle = millis();
        blinkers[idx].state = false;
    }
}

void checkRebootCombo() {
    if (config.buttons[0].pinInput != -1 && config.buttons[1].pinInput != -1) {
        if (digitalRead(config.buttons[0].pinInput) == LOW && 
            digitalRead(config.buttons[1].pinInput) == LOW) {
            if (hasDisplay) { display.clearDisplay(); display.display(); }
            delay(500); 
            ESP.restart();
        }
    }
}

// --- HARDWARE-LOOP ---
void updateHardware() {
    checkRebootCombo();

    // LED Blinken abarbeiten
    for (int i = 0; i < 8; i++) {
        if (blinkers[i].count > 0 && millis() >= blinkers[i].nextToggle) {
            blinkers[i].state = !blinkers[i].state;
            digitalWrite(blinkers[i].pin, blinkers[i].state);
            blinkers[i].count--;
            blinkers[i].nextToggle = millis() + 100;
        }
    }

    // Display Timeout
    if (displayActive && (millis() - displayTimeout > 15000)) {
        displayActive = false;
        updateDisplay();
    }
}