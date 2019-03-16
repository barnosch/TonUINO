# Barnoschs TonUINO Version

- 5 Tasten Mod & LED Status
- 2 Zusätzliche Taster für die Lautstärke (A3 und A4)
- Pause: LED ist aus
- Play : LED fadet ein und aus

# LED Ring Version - StatusLED - DEV Version
 -Tonuino_2.1_LEDRing_StatusLED.ino-
 
<FastLED.h> und <Adafruit_NeoPixel.h> Bibliotheken werden zusätzlich für den LED Ring benötigt
Nun alles Optional definierbar, siehe Code
- 5 Tasten - 2 Zusätzliche Taster für die Lautstärke (A3 und A4)
- StatusLED - PIN 5 (fadet bei Pause, AUS bei Play)
- Lauter/Next LED (AUS bei Pause, AN bei Play), blinkt kurz bei Lauter
- Leiser/Previous LED (AUS bei Pause, AN bei Play), blinkt kurz bei Leiser
- 24 LED WS2812 NeoPixel Ring mit wechselnder Animation, während des Abspielens - PIN 6
