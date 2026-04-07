#include "hid_ble.h"
#include "src/ESP32-BLE-Combo-master/BleCombo.h"
#include "config_manager.h"
#include "hardware_io.h"

// Die Instanz muss exakt so heißen wie im Example
BleCombo bleCombo;
bool bleConnected = false;

void initBLE() {
    Serial.printf(">>> [BLE] Initialisiere mit Name: %s\n", config.deviceName);
    // Wir lassen setName weg, da es in der Lib oft hardcodiert ist
    bleCombo.setName(std::string(config.deviceName));
    bleCombo.begin();
}

void sendAction(uint16_t code, uint8_t modifiers, int btnIdx) {
    if (!bleCombo.isConnected()) return;

    // Lokale Szenenwechsel (2000-2004)
    if (code >= 2000 && code <= 2004) {
        currentSceneIdx = code - 2000; updateDisplay();
        if (btnIdx != -1) startBlink(btnIdx); return;
    }

    // --- TASTATUR (ASCII & Sondertasten) ---
    if (code > 0 && code < 300) {
        uint8_t finalChar = (uint8_t)code;

        // Falls es ein Kleinbuchstabe ist (97-122) UND Shift aktiv ist (0x02)
        if ((modifiers & 0x02) && (finalChar >= 97 && finalChar <= 122)) {
            finalChar -= 32; // Macht aus 'a' (97) ein 'A' (65)
            // Wir löschen das Shift-Bit aus den Modifiers für diesen Aufruf,
            // da die Library bei 'A' (65) das Shift selbst drückt!
            modifiers &= ~0x02; 
        }

        // Manuelle Modifier drücken (Strg, Alt, GUI)
        if (modifiers & 0x01) bleCombo.press(KEY_LEFT_CTRL);
        if (modifiers & 0x02) bleCombo.press(KEY_LEFT_SHIFT); // Nur falls nicht für A-Z verbraucht
        if (modifiers & 0x04) bleCombo.press(KEY_LEFT_ALT);
        if (modifiers & 0x08) bleCombo.press(KEY_LEFT_GUI);

        // Das Zeichen senden
        bleCombo.write(finalChar);
        
        delay(10);
        bleCombo.releaseAll();
    }
    
    // --- MEDIA KEYS (400er Bereich) ---
    else if (code >= 401 && code <= 406) {
        switch(code) {
            case 404: bleCombo.write(KEY_MEDIA_PLAY_PAUSE); break;
            case 401: bleCombo.write(KEY_MEDIA_VOLUME_UP); break;
            case 402: bleCombo.write(KEY_MEDIA_VOLUME_DOWN); break;
            case 403: bleCombo.write(KEY_MEDIA_MUTE); break;
            case 405: bleCombo.write(KEY_MEDIA_NEXT_TRACK); break;
            case 406: bleCombo.write(KEY_MEDIA_PREVIOUS_TRACK); break;
        }
    }

    // --- MAUS (3000er Bereich) ---
    else if (code >= 3001 && code <= 3004) {
        switch(code) {
            case 3003: bleCombo.click(MOUSE_LEFT); break;
            case 3004: bleCombo.click(MOUSE_RIGHT); break;
            case 3001: bleCombo.move(0, 0, 1); break;
            case 3002: bleCombo.move(0, 0, -1); break;
        }
    }

    if (btnIdx != -1) startBlink(btnIdx);
}