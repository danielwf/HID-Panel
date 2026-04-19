#include <Arduino.h>
#include <nvs_flash.h>
#define ONBOARD_LED 2
#include "config_manager.h"
#include "hardware_io.h"
#include "wifi_portal.h"
#include "hid_ble.h"

const char* currentVersion = "260419A";


// Hilfsfunktion für das Faden (Sinus-basiert für weichen Übergang)
void fadeLEDs(int pin1, int pin2, unsigned long ms) {
    if (pin1 == -1 && pin2 == -1) return;
    float brightness = (sin(ms / 500.0 * PI) + 1) * 127.5; 
    if (pin1 != -1) analogWrite(pin1, (int)brightness);
    if (pin2 != -1) analogWrite(pin2, (int)brightness);
}

void setup() {
    Serial.begin(115200);
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        Serial.println("NVS Repair...");
        nvs_flash_erase();
        ret = nvs_flash_init();
    } 
    pinMode(ONBOARD_LED, OUTPUT);
    digitalWrite(ONBOARD_LED, HIGH); 

    if (!LittleFS.begin(false)) { 
        Serial.println("LittleFS Mount failed, trying format...");
        if(!LittleFS.begin(true)) Serial.println("LittleFS Fatal Error!");
    }
    bool hasConfig = loadConfig();

    initHardware(); 

    bool wlanReq = !hasConfig; 
    int led1 = config.buttons[0].pinLED;
    int led2 = config.buttons[1].pinLED;
    
    if (!wlanReq) {
        int p1 = config.buttons[0].pinInput;
        if (p1 != -1) {
            pinMode(p1, INPUT_PULLUP);
            unsigned long start = millis();
            
            // Display-Anzeige (optional)
            display.clearDisplay();
            display.setTextSize(1);
            display.setTextColor(WHITE);
            display.setCursor(10, 10);  display.println("Taste 1: Wifi-Cfg");
            display.setCursor(10, 25);  display.println("Taste 1+2: Reset");
            display.setCursor(10, 45);  display.println("Warte 5s...");
            display.display();

            while (millis() - start < 5000) {
                // LED 1 fadet während der Wartezeit
                fadeLEDs(led1, -1, millis());

                if (digitalRead(p1) == LOW) {
                    wlanReq = true;
                    break;
                }
                delay(10);
            }
            // LEDs nach Wartezeit aus
            if (led1 != -1) {
                analogWrite(led1, 0); 
                digitalWrite(led1, LOW);
            }
        }
    }

    if (wlanReq) {
        startWifiPortal(); 
        configModeActive = true; 
        
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(10, 10);  display.printf("WIFI: %s", ssid);
        display.setCursor(10, 25);  display.printf("PW:   %s", pass);
        display.setCursor(10, 40);  display.println("IP: 192.168.4.1");
        display.display();
        
        displayActive = true;
        displayTimeout = millis() + 3600000; 
        
        // Im WLAN-Modus faden LED 1 und LED 2 in der loop() (siehe unten)
    } else {
        digitalWrite(ONBOARD_LED, LOW); 
        if (led1 != -1) analogWrite(led1, 0);
        if (led2 != -1) analogWrite(led2, 0);
        if (led1 != -1) pinMode(led1, OUTPUT);
        if (led2 != -1) pinMode(led2, OUTPUT);
        
        // Kurzes Aufleuchten von LED 2 beim BT-Start
        if (led2 != -1) {
            digitalWrite(led2, HIGH);
            delay(200);
            digitalWrite(led2, LOW);
            delay(200);
            digitalWrite(led2, HIGH);
            delay(200);
            digitalWrite(led2, LOW);
        }

        initBLE(); 
        delay(100);
        updateDisplay(); 
    }
}

void loop() {
    updateHardware(); 

    if (configModeActive) {
        handlePortal();
        // LED 1 & 2 faden gemeinsam im WiFi-Modus
        fadeLEDs(config.buttons[0].pinLED, config.buttons[1].pinLED, millis());
    } else {
        // --- 1. Buttons ---
        static bool lastBtnState[8] = {0};
        static unsigned long lastBtnTime[8] = {0}; 
        const unsigned long debounceDelay = 50;    

        for (int i = 0; i < 8; i++) {
            int p = config.buttons[i].pinInput;
            int led = config.buttons[i].pinLED;
            if (p == -1) continue;
            
            bool currentState = (digitalRead(p) == LOW); 
            
            if (currentState != lastBtnState[i]) {
                if ((millis() - lastBtnTime[i]) > debounceDelay) {
                    
                    if (config.buttons[i].isSwitch) {
                        if (led != -1) digitalWrite(led, currentState ? HIGH : LOW);

                        if (currentState) {
                            sendAction(config.scenes[currentSceneIdx].btnK[i], 
                                       config.scenes[currentSceneIdx].btnM[i], i);
                        } else {
                            sendAction(config.scenes[currentSceneIdx].btnOffK[i], 
                                       config.scenes[currentSceneIdx].btnOffM[i], i);
                        }
                    } else {
                        if (currentState) {
                            sendAction(config.scenes[currentSceneIdx].btnK[i], 
                                       config.scenes[currentSceneIdx].btnM[i], i);
                            startBlink(i);
                        }
                    }

                    displayActive = true;
                    displayTimeout = millis();
                    updateDisplay();
                    
                    lastBtnTime[i] = millis();
                }
                lastBtnState[i] = currentState;
            }
        }

        // --- 2. Encoder ---
        static unsigned long lastSpeedCheck = 0;
        static long lastTicks = 0;
        static unsigned long lastFastAction = 0; 

        if (millis() - lastSpeedCheck >= 100) { 
            long currentTicks = encoderTicks;
            long diffTicks = currentTicks - lastTicks; 
            lastTicks = currentTicks;
            lastSpeedCheck = millis();

            if (diffTicks != 0) {
                float degreesMoved = (abs(diffTicks) / (float)config.stepsPerRev) * 360.0;
                float speedLookAhead = degreesMoved * 5.0;

                int dirIdx = 0;
                bool actionTriggered = false;

                if (speedLookAhead >= (float)config.speedThreshold) {
                    if (millis() - lastFastAction >= 500) {
                        dirIdx = (diffTicks > 0) ? 3 : 2; 
                        sendAction(config.scenes[currentSceneIdx].encK[dirIdx], 
                                config.scenes[currentSceneIdx].encM[dirIdx]);
                        
                        lastFastAction = millis(); 
                        actionTriggered = true;
                    }
                } 
                else {
                    int numActions = abs(diffTicks) / config.stepsSlow;
                    if (numActions > 0) {
                        dirIdx = (diffTicks > 0) ? 1 : 0; 
                        for (int k = 0; k < numActions; k++) {
                            sendAction(config.scenes[currentSceneIdx].encK[dirIdx], 
                                    config.scenes[currentSceneIdx].encM[dirIdx]);
                        }
                        actionTriggered = true;
                    }
                }

                if (actionTriggered) {
                    displayActive = true; 
                    displayTimeout = millis(); 
                    updateDisplay();
                }
            }
        }
    }
}