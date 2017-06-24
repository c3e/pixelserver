# pixelserver

Dieses Projekt soll als Backend für die Deckensteuerung in Kombiantion mit dem Projekt pixeldecke dienen. Der Server verbindet sich via serial mit dem teensy und nimmt auf verschiedenen Schnittstellen ( http, pixelflut, mqtt, ... ) Kommandos entgegen.

## Dependencies
Arch:

   ```
   pacman -S mosquitto libevent
   ```

Debian/Ubuntu:

   ```
   apt-get install mosquitto libevent2
   ```

## Installation

   ```
   git clone https://github.com/c3e/pixelserver
   cd https://github.com/c3e/pixelserver
   mkdir build
   cmake ..
   make
   ```

## TODO

-	mqtt endpoint api
-	initiale Konfiguration
- 	Dokumentation