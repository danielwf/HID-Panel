#include "hid_ble.h"
#include <HijelHID_BLEKeyboard.h>
#include "config_manager.h"
#include "hardware_io.h"

HijelHID_BLEKeyboard keyboard;

void initBLE() {
    keyboard = HijelHID_BLEKeyboard(config.deviceName, "DanielWF");
    keyboard.setLogLevel(HIDLogLevel::Normal);
    keyboard.begin();
    Serial.println(">>> [BLE] Init abgeschlossen.");
}

void checkSleep() {
    if (keyboard.isPaired() && keyboard.getIdleTime() >= 20000) {
        goToSleep();
    }
}

void goToSleep() {
    Serial.println(F(">>> [BLE] Idle Timeout -> Light Sleep"));
    
    keyboard.beforeSleep(); 

    // Wir registrieren alle belegten Pins als Weckquelle
    uint64_t mask = 0;
    for (int i = 0; i < 8; i++) {
        if (config.buttons[i].pinInput != -1 && config.buttons[i].pinInput <= 39) {
            mask |= (1ULL << config.buttons[i].pinInput);
        }
    }

    // ESP32 Hardware-Einschränkung: 
    // ESP_EXT1_WAKEUP_ANY_LOW ist der korrekte Name für "Einer der Pins geht auf LOW"
    // (In manchen ESP-Versionen heißt es ESP_EXT1_WAKEUP_ALL_LOW bei nur einem Pin, 
    // aber wir nehmen die sicherste Variante für Taster):
    esp_sleep_enable_ext1_wakeup(mask, ESP_EXT1_WAKEUP_ALL_LOW);

    // Jetzt schlafen legen
    esp_light_sleep_start();

    // Nach dem Aufwachen:
    keyboard.afterWake();
    Serial.println(F(">>> [BLE] Wach!"));
}

void sendAction(uint16_t code, uint8_t modifiers, int btnIdx) {
    // 1. Lokale Szenenwechsel (2000-2004)
    if (code >= 2000 && code <= 2004) {
        currentSceneIdx = code - 2000;
        updateDisplay();
        if (btnIdx != -1) startBlink(btnIdx);
        return; 
    }

    if (!keyboard.isPaired()) return;

    // 2. MODIFIER MAPPING (Exakt nach wifi_portal.cpp)
    uint8_t activeMods = 0;
    if (modifiers & 1) activeMods |= KEY_MOD_LCTRL;  // Bit 0: Strg
    if (modifiers & 2) activeMods |= KEY_MOD_LSHIFT; // Bit 1: Shift
    if (modifiers & 4) activeMods |= KEY_MOD_LALT;   // Bit 2: Alt
    if (modifiers & 8) activeMods |= KEY_MOD_LGUI;   // Bit 3: Win

    // 3. TASTEN-LOGIK
    uint8_t hidCode = 0;
    bool isMedia = false;

    // Spezial-IDs aus deiner config_manager.cpp Mapping-Tabelle
    switch(code) {
        case 176: hidCode = KEY_RETURN; break;
        case 177: hidCode = KEY_ESCAPE; break;
        case 178: hidCode = KEY_BACKSPACE; break;
        case 179: hidCode = KEY_TAB; break;
        case 215: hidCode = KEY_RIGHT; break;
        case 216: hidCode = KEY_LEFT; break;
        case 217: hidCode = KEY_DOWN; break;
        case 218: hidCode = KEY_UP; break;
        case 209: hidCode = KEY_INSERT; break;
        case 212: hidCode = KEY_DELETE; break;
        case 210: hidCode = KEY_HOME; break;
        case 213: hidCode = KEY_END; break;
        case 211: hidCode = KEY_PAGE_UP; break;
        case 214: hidCode = KEY_PAGE_DOWN; break;
        // F-Tasten (194-205)
        case 194 ... 205: hidCode = (KEY_F1 + (code - 194)); break;
        // Media Keys
        case 401 ... 406: isMedia = true; break;
    }

    // 4. AUSFÜHRUNG
    if (isMedia) {
        uint16_t mKey = 0;
        switch(code) {
            case 401: mKey = MEDIA_VOLUME_UP; break;
            case 402: mKey = MEDIA_VOLUME_DOWN; break;
            case 403: mKey = MEDIA_MUTE; break;
            case 404: mKey = MEDIA_PLAY_PAUSE; break;
            case 405: mKey = MEDIA_NEXT_TRACK; break;
            case 406: mKey = MEDIA_PREV_TRACK; break;
        }
        keyboard.tap(mKey);
    } 
    else if (hidCode != 0) {
        // Navigation & Sondertasten
        keyboard.press(hidCode, activeMods);
        delay(35); // Haltezeit für den PC
        keyboard.releaseAll();
    } 
    else if (code >= 32 && code <= 122) {
        // Buchstaben & Zahlen (ASCII Bereich)
        // Falls Shift im Web gesetzt ist ODER es ein Großbuchstabe (65-90) ist:
        if ((code >= 65 && code <= 90)) {
            activeMods |= KEY_MOD_LSHIFT;
        }

        // Wir berechnen den HID-Code für A-Z/a-z direkt (A/a = 0x04)
        uint8_t letterHid = 0;
        if (code >= 65 && code <= 90) letterHid = (code - 65) + 0x04;
        else if (code >= 97 && code <= 122) letterHid = (code - 97) + 0x04;
        else if (code >= 48 && code <= 57) letterHid = (code == 48) ? 0x27 : (code - 49) + 0x1E; // 0-9
        else if (code == 32) letterHid = KEY_SPACE;

        if (letterHid != 0) {
            keyboard.press(letterHid, activeMods);
            delay(35);
            keyboard.releaseAll();
        } else {
            // Letzter Rettungsweg für Sonderzeichen
            keyboard.write((uint8_t)code);
        }
    }

    keyboard.releaseAll(); // Sicherheitshalber
    if (btnIdx != -1) startBlink(btnIdx);
}