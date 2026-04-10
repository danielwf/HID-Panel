
## Programmierung
Am einfachsten ist folgender Weg:  
Im Ordner "bin" liegen mehrere *.bin-Dateien, die sich hiermit auf einen neuen ESP32-D1-Mini flashen lassen:
https://espressif.github.io/esptool-js/ (Chromium/Edge/Vivaldi/Chrome-Browser benötigt)  
Die Startadressen für die einzelnen Speicherbereiche sind den Dateinamen zu entnehmen.  
Für spätere OTA-Updates über das Webinterface des Gerätes ist die Datei "0x10000_application-OTA.bin" ausreichend. 

aktuelle Version 260410A
