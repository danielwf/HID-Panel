#ifndef HID_BLE_H
#define HID_BLE_H

#include <Arduino.h>

void initBLE();
void sendAction(uint16_t code, uint8_t modifiers = 0, int btnIdx = -1);

extern bool bleConnected;

#endif