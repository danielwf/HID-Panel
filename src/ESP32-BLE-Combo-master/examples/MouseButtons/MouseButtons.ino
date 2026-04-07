/**
 * This example turns the ESP32 into a Bluetooth LE mouse that continuously moves the mouse.
 */
#include <BleCombo.h>

BleCombo bleCombo;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE work!");
  bleCombo.begin();
}

void loop() {
  if(bleCombo.isConnected()) {
    Serial.println("Left click");
    bleCombo.click(MOUSE_LEFT);
    delay(500);

    Serial.println("Right click");
    bleCombo.click(MOUSE_RIGHT);
    delay(500);

    Serial.println("Scroll wheel click");
    bleCombo.click(MOUSE_MIDDLE);
    delay(500);

    Serial.println("Back button click");
    bleCombo.click(MOUSE_BACK);
    delay(500);

    Serial.println("Forward button click");
    bleCombo.click(MOUSE_FORWARD);
    delay(500);

    Serial.println("Click left+right mouse button at the same time");
    bleCombo.click(MOUSE_LEFT | MOUSE_RIGHT);
    delay(500);

    Serial.println("Click left+right mouse button and scroll wheel at the same time");
    bleCombo.click(MOUSE_LEFT | MOUSE_RIGHT | MOUSE_MIDDLE);
    delay(500);

  }
}