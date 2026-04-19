# HID-Panel
![Overview with wooden panel and Arcade-Buttons, 3D-printed Case and Phone displaying the config interface](HID-Panels.png)

**Derzeit bekannte Fehler, die aktuell behoben werden:**
- Keine Funktion mit iOS
- OTA-Updates ohne Rückmeldung
- Kein Werksreset implementiert
- Tastatur/Maus-Combo macht mit manchen Systemen Probleme, so dass aktuell auf reine Tastaturemulation umgestellt wird  
--> **Update folgt noch bis Ende April!**

-----

**Bluetooth-HID-Emulation with ESP32, configuration per WIFI-Portal**  
Since this project is also aimed at German-speaking youth work, the German language is used here. Please use your browser's translation function if you need this in English or another language ;)  

**Bluetooth-HID-Emulation mit ESP32, frei konfigurierbar per WIFI-Portal**  

Ziel dieses Projektes ist, mit einem ESP32 und ein paar Tasten, Schalter und Drehencoder einen Tastaturemulator zu bauen.  
Die Konfiguration erfolgt über ein Webinterface, das eine zusätzliche App überflüssig macht.  
Das Gerät meldet sich bei normalem Start als Bluetooth-Tastatur (mit Medientasten), die sich mit jedem Gerät koppeln lässt (Windows, Linux, MacOs, Android, iOS).  
**Der Code wurde mit wesentlicher Hilfe von Google Gemini erstellt.**   
(Natürlich ist das unschön, allerdings hat es mich so nur ein paar Urlaubstage und nicht mehrere Urlaubswochen gekostet. Wer einen ähnlichen Code handgeschrieben hinbekommt und mir ein Update schicken möchte, kann das natürlich gerne machen...)  

## Hardware
Entwickelt wurde das Programm auf einem "ESP32 D1 Mini", da diese Boards sehr kompakt sind. Grundsätzlich sollten aber auch alle anderen "ESP32-Wroom-32"-Platinen funktionieren.  

- Alle Taster und Schalter müssen gegen GND verbunden werden. Die Eingänge werden mit den im ESP32 integrierten PullUp-Widerständen gelesen.  
   - Es sind mindestens zwei Taster erforderlich (zum Start der WIFI-Configuration und für den Reset). 
   - Es können nicht alle Anschlüsse des ESP32 benutzt werden. Die benutzten Anschlüsse sind aber selbst einzustellen. 
     Empfehlungen für die Anschlüsse sind in der Tabelle unten zu finden. 

- Die Verwendung eines OLED-LCDs (SSD1306) ist optional:
   - Anschlüsse: SDA-->GPIO21 ; SCL-->GPIO22
   - I2C-Adresse 0x3C, also so wie bei diesen OLEDs üblich  
   (nicht vom aufgedruckten 0x78 irritieren lassen, 0x3c ist tatsächlich meist richtig)

- Die Verwendung eines Drehencoder ist optional. Verwendet werden können:
   - optischer Inkremental-Drehgeber "600P/R", 600 Schritte/U (unbedingte Empfehlung)
      - WICHTIG: VCC am Drehgeber mit 5V am ESP32 verbinden
        Die Ausgänge der optischen Drehgeber sind spannungslose "OpenCollector"-Ausgänge, daher ist das kein Problem mit dem 3.3V-Controller.
      - und am ESP32 die Diode D1 (zwischen USB und 5V) überbrücken. 
   - mechanischer Drehgeber "KY-040", 20 Schritte/U (funktioniert natürlich auch)
      - eingebauter Taster am "SW-Pin" als beliebigen Taster behandeln.
   - empfohlene Anschlüsse: A/CLK-->GPIO32 ; B/DT-->GPIO12
 
Beispiel(!) für Anschlussbelegung am ESP32:
| Funktion       | GPIO | Bemerkung                                |
|----------------|------|------------------------------------------|
|Taste 1         |  14  | Taste 1, wird für WIFI-Config benötigt   |
|LED 1           |  13  | LED von Taste 1, z.B. bei Arcade-Button - leuchtet bei BootUp/Wifi-Config  |
|Taste 2         |  16  | Taste 2, mit Taste 1 für Reset - leuchtet im WIFI-Config |
|LED 2           |  17  | LED von Taste 2                          |
|Taste/Schalter 3|  18  | Taste/Schalter 3                         |
|LED 3           |  19  | LED von Taste/Schalter 3                 |
|Taste/Schalter 4|  25  | Taste/Schalter 4                         |
|LED 4           |  26  | LED von Taste/Schalter 4                 |
|Taste/Schalter 5|  27  | Taste/Schalter 5                         |
|LED 5           |  33  | LED von Taste/Schalter 5                 |
|Taste/Schalter 6|  23  | Taste/Schalter 6                         |
|LED 6           |  05  | LED von Taste/Schalter 6                 |
|Taste/Schalter 7|  15  | Taste/Schalter 7                         |
|LED 7           |  04  | LED von Taste/Schalter 7                 |
|Taste/Schalter 8|  00  | Taste/Schalter 8                         |
|LED 8           |  02  | LED von Taste/Schalter 8                 |
|Display SDA     |  21  | (Display ist optional)                   |
|Display SCL     |  22  |                                          |
|Display VCC     | 3.3V |                                          |
|Display GND     | GND  |                                          |
|Drehgeber A/CLK |  32  | (Drehgeber ist optional, wird empfohlen) |
|Drehgeber B/DT  |  12  |                                          |
|Drehgeber VCC   | 3.3V | bei mechanischen Drehgeber KY-040        |
|Drehgeber VCC   | 5V   | bei optischen Drehgeber 600P/R, D1 überbrücken! |
|Drehgeber GND   | GND  |                                          |
|Drehgeber SW    |  ->  | wenn vorhanden: wie eine der oben genannten Tasten verbinden |

Die o.g. GPIOs können beliebig verwendet und später im Webinterface konfiguriert werden. Die o.g. Tabelle beschreibt nur ein Beispiel.  
Es müssen mindestens zwei Tasten verbunden werden. LEDs, Display und Drehgeber sind immer optional!  
Das Pinout ist z.B. hier zu finden: https://www.espboards.dev/esp32/d1-mini32/  


## Programmierung
Am einfachsten ist das **Flashen der bin-Dateien über den Browser**:  
   Im Ordner "bin" liegen mehrere *.bin-Dateien, die sich hiermit auf einen neuen ESP32 flashen lassen:  
   https://espressif.github.io/esptool-js/ (Chromium/Edge/Vivaldi/Chrome-Browser benötigt)  
   Die Startadressen für die einzelnen Speicherbereiche sind den Dateinamen zu entnehmen.  
   Für spätere OTA-Updates über das Webinterface des Gerätes ist die Datei "0x10000_application-OTA.bin" ausreichend.  
   Nach Flashen der bin-Dateien muss der ESP32 einmal vom Strom/USB-Anschluss getrennt und neu gestartet werden.  

Für die **Programmierung über die Arduino-IDE** (2.3.6 oder höher) werden folgende Abhängigkeiten benötigt: 
   - per Board Manager: Arduino ESP32-Boards (by Arduino)
   - per Library Manager:
      - "ESP Async WebServer" (by ESPAsync)
      - "ESP Async TCP" (by ESPAsync)
      - "Adafruit_SSD1306" (by Adafruit)
      - "HijelHID_BLEKEyboard (by Hijel)

## Konfiguration
Die Konfiguration erfolgt über ein Webinterface, das beim Erststart automatisch gestartet wird. (SSID: HID-Panel-Cfg - PW: 12345678 ).  
In jedem Einstellungsbereich gibt es ganz unten einen "Speichern"-Button. Wird der Einstellungsbereich gewechselt, ohne zu speichern, werden die Einstellungen nicht übernommen.
Ein Display ist optional, wird automatisch erkannt und muss nicht konfiguriert werden. Es wird nur das SSD1306-OLED-Display unterstützt. 

- Unter "Hardware" wird der Bluetooth-Name festgelegt. Diesen sollte man später nicht ändern (bzw. zuerst das Gerät entkoppeln, bevor man den Namen ändert).  
  Ebenfalls werden dort die Anschlüsse eines Drehgebers, der Taster und Schalter festgelegt, ggf. auch mit den GPIOS für die passenden LEDs (z.B. bei Arcade-Buttons mit eingebauten LEDs).  
   **ACHTUNG:**
   - **Der erste konfigurierte Taster ist der Button, mit dem das WIFI-Interface gestartet wird (innerhalb der ersten 5 Sekunden nach Start/Reset).**  
   - **Der zweite konfigurierte Taster wird zusammen mit dem ersten Taster gedrückt, um das Gerät neu zu starten.**  
  Diese sollten also entsprechend zuerst eingestellt werden. Das heißt auch, dass mindestens zwei Taster erforderlich sind. 
   - Es lassen sich bis zu 8 Taster konfigurieren, ggf. auch als Schalter. Anschlüsse, die mit "-1" als GPIO angegeben wurden, werden nicht benutzt. 

- Ein Drehgeber ist optional.  
  Unter "Hardware" wird eingestellt, auf welchen GPIOS A/CLK und B/DTR angeschlossen werden.  
  Empfohlene Einstellungen:
  - bei mechanischen Drehgebern wie z.B. einem KY-040: 20 Schritte/U; 1 Schritte (Fein); 1000° Winkel/s (Fast)  
  - bei optischen Drehgebern wie z.B. einem 600P/R: 600 Schritte/U; 20 Schritte (Fein); 360° Winkel/s (Fast)
  - Wer die Erkennung von schnellen Drehungen nicht benötigt oder störend empfindet, stellt einfach einen sehr hohen Wert für Winkel/s ein (z.B. 5000)

- Unter "Szenen" lassen sich bis zu 5 verschiedene Layout-Szenarien festlegen (Beispiele: Media, Scroll, Game, Video, Funk)  
  Die Umschaltung der Szenen lässt sich wie alle Kommandos auf beliebige Buttons und Schalter legen (ganz unten in der jeweiligen Auswahl). 

- Unter "Rot" wird pro Szene festgelegt, wie der Drehgeber sich verhält, bzw. welche Kommandos geschickt werden.  
  Konfiguriert werden können die L- und R-Befehle bei normaler/langsamer Drehung und bei schneller Drehung.  
  Beispiel: Bei langsamer Drehung könnte hier die Lautstärke, bei schneller Drehung der letzte oder nächste Titel gewählt werden.  
  Beispiel 2: Bei Verwendung für eine Funksoftware könnten z.B. getrennte Tastenbefehle für 100Hz- und 10Hz-Schritte eingestellt werden.  

- Unter "SW" werden pro Szene die Schalter konfiguriert.  
  Je Schalteränderung auf ein oder aus lassen sich unterschiedliche Tastenbefehle verknüpfen.  
  Eignet sich z.B. auch super für die Umschaltung von Szenen (z.B. Aus=Mediensteuerung und An=Gaming o.ä.)  

- Unter "Keys" werden pro Szene die Taster konfiguriert.  
  Bei Verwendung eines Displays empfielt es sich, eine selbst festgelegte Taste in allen Szenen so zu konfigurieren, dass diese zur jeweils nächsten Szene umschaltet - und von der letzten natürlich wieder zur ersten Szene ;)  

## 3D-Druck-Vorlage
Die im Projekt integrierten 3D-Vorlagen enthalten einmal den Drehknopf passend für einen 600P/R-Drehgeber und das Gehäuse mit allen Haltern.  
Das Gehäuse ist sowohl für ESP32-D1-Mini wie auch für den Raspi Pico geeignet. Der Ausschnitt ist für ein 0.96"-OLED-Display (SSD1306) vorbereitet.  
Das Gehäuse ist so entworfen worden, dass kein 3D-Druck mit Support nötig ist.  
Der Halter im Displayausschnitt lässt sich leicht entfernen.  
Falls jemand eine Klinkenbuchse für einen Fußtaster/Tätowiertaster installieren möchte, gibt es einen flachen Bereich mit "Vorkörnung" im Deckel ;)  
Die Teile werden mit M3-Schrauben und Muttern montiert. Schaut am besten in Eurem M3-Sortimentskasten, was für Euch am besten passt. Die Schrauben für die Montage des Drehgebers sollten allerdings möglichst kurz sein.  
Falls der Drehknopf auf dem Gehäuse schleift, ist es ausreichend, einen rundgebogenen 0.8mm Kupferlackdraht unter dem Knopf auf die Achse zu stecken.  
**Dieses Gehäuse ist für das Projekt nicht unbedingt erforderlich!**
Es kann natürlich jedes beliebige Holzbrett, Holzkiste oder Blechgehäuse verwendet werden. 
Wer allerdings ein formschönes Gehäuse benötigt und einen 3D-Drucker hat, hat hier eine gute Vorlage. 
![](3DPrint.png)
![](3d-Print.png)
