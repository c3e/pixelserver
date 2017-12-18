# pixelserver

Dieses Projekt soll als Backend für die Deckensteuerung in Kombiantion mit dem Projekt pixeldecke dienen. Der Server verbindet sich via serial mit dem teensy und nimmt auf verschiedenen Schnittstellen ( http, pixelflut, mqtt, ... ) Kommandos entgegen.

## Dependencies
Arch:

   ```
   pacman -S mosquitto libevent rapidjson
   ```

Debian/Ubuntu:

   ```
   apt-get install mosquitto libevent2 rapidjson 
   ```

## Installation

   ```
   git clone https://github.com/c3e/pixelserver
   cd https://github.com/c3e/pixelserver
   mkdir build
   cmake ..
   make
   ```
## Status

Momentan ist die Konfiguration noch statisch und es läuft out of the box mit einem simplen 3x3 Setup ( Mehr ist der main() zu entnehmen. 


## TODO

-	Konfigurationsdateien
- 	Dokumentation
