/* Barnosch LED Ring Mod
5 Tasten + LED Ring (12) MOD
*/
#include <DFMiniMp3.h>
#include <EEPROM.h>
#include <JC_Button.h>
#include <MFRC522.h>
#include <SPI.h>
#include <SoftwareSerial.h>

#include <FastLED.h> // FastLED-Library einbinden

#include <Adafruit_NeoPixel.h> Adafruit Neopixel-Library einbinden
  #ifdef __AVR__
  #include <avr/power.h>
  #endif
  #define PIN 6
  #define NUM_LEDS 12
  Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);


// FastLED define und Brightness für Adafruit Neopixel
  FASTLED_USING_NAMESPACE
  #define DATA_PIN    6
  //#define CLK_PIN   4
  #define LED_TYPE    WS2812
  #define COLOR_ORDER RGB
  #define NUM_LEDS    12
  CRGB leds[NUM_LEDS];
  #define BRIGHTNESS1          3  // Helligkeit für FastLED Low (wenn z.B. KEINE Musik gespielt wird)
  #define BRIGHTNESS2          30 // Helligkeit für FastLED High (wenn z.B Musik gespielt wird)
  #define BRIGHTNESS3          50 // Helligkeit für FastLED bei Bestätigung z.B. Blinken o.ä.
  #define STARTBRIGHTNESS      50 // Helligkeit für ADAFRUIT-Library
  #define FRAMES_PER_SECOND  120 
  uint8_t gHue = 0; // rotating "base color" used by many of the patterns

// DFPlayer Mini
SoftwareSerial mySoftwareSerial(2, 3); // RX, TX
uint16_t numTracksInFolder;
uint16_t currentTrack;
uint8_t volume;                      // Volume Variable
uint8_t minVolume;
uint8_t maxVolume;

// this object stores nfc tag data
struct nfcTagObject {
  uint32_t cookie;
  uint8_t version;
  uint8_t folder;
  uint8_t mode;
  uint8_t special;
};

nfcTagObject myCard;

static void nextTrack(uint16_t track);
int voiceMenu(int numberOfOptions, int startMessage, int messageOffset,
              bool preview = false, int previewFromFolder = 0);

bool knownCard = false;

// implement a notification class,
// its member methods will get called
//
class Mp3Notify {
  public:
    static void OnError(uint16_t errorCode) {
      // see DfMp3_Error for code meaning
      Serial.println();
      Serial.print("Com Error ");
      Serial.println(errorCode);
    }
    static void OnPlayFinished(uint16_t track) {
      Serial.print("Track beendet");
      Serial.println(track);
      delay(100);
      nextTrack(track);
    }
    static void OnCardOnline(uint16_t code) {
      Serial.println(F("SD Karte online "));
    }
    static void OnCardInserted(uint16_t code) {
      Serial.println(F("SD Karte bereit "));
    }
    static void OnCardRemoved(uint16_t code) {
      Serial.println(F("SD Karte entfernt "));
    }
};

static DFMiniMp3<SoftwareSerial, Mp3Notify> mp3(mySoftwareSerial);

// Leider kann das Modul keine Queue abspielen.
static uint16_t _lastTrackFinished;
static void nextTrack(uint16_t track) {
  if (track == _lastTrackFinished) {
    return;
  }
  _lastTrackFinished = track;

  if (knownCard == false)
    // Wenn eine neue Karte angelernt wird soll das Ende eines Tracks nicht
    // verarbeitet werden
    return;

  if (myCard.mode == 1) {
    Serial.println(F("Hörspielmodus ist aktiv -> keinen neuen Track spielen"));
    //    mp3.sleep(); // Je nach Modul kommt es nicht mehr zurück aus dem Sleep!
  }
  if (myCard.mode == 2) {
    if (currentTrack != numTracksInFolder) {
      currentTrack = currentTrack + 1;
      mp3.playFolderTrack(myCard.folder, currentTrack);
      Serial.print(F("Albummodus ist aktiv -> nächster Track: "));
      Serial.print(currentTrack);
    } else
      //      mp3.sleep();   // Je nach Modul kommt es nicht mehr zurück aus dem Sleep!
    { }
  }
  if (myCard.mode == 3) {
    currentTrack = random(1, numTracksInFolder + 1);
    Serial.print(F("Party Modus ist aktiv -> zufälligen Track spielen: "));
    Serial.println(currentTrack);
    mp3.playFolderTrack(myCard.folder, currentTrack);
  }
  if (myCard.mode == 4) {
    Serial.println(F("Einzel Modus aktiv -> Strom sparen"));
    //    mp3.sleep();      // Je nach Modul kommt es nicht mehr zurück aus dem Sleep!
  }
  if (myCard.mode == 5) {
    if (currentTrack != numTracksInFolder) {
      currentTrack = currentTrack + 1;
      Serial.print(F("Hörbuch Modus ist aktiv -> nächster Track und "
                     "Fortschritt speichern"));
      Serial.println(currentTrack);
      mp3.playFolderTrack(myCard.folder, currentTrack);
      // Fortschritt im EEPROM abspeichern
      EEPROM.write(myCard.folder, currentTrack);
    } else {
      //      mp3.sleep();  // Je nach Modul kommt es nicht mehr zurück aus dem Sleep!
      // Fortschritt zurück setzen
      EEPROM.write(myCard.folder, 1);
    }
  }
}

static void previousTrack() {
  if (myCard.mode == 1) {
    Serial.println(F("Hörspielmodus ist aktiv -> Track von vorne spielen"));
    mp3.playFolderTrack(myCard.folder, currentTrack);
  }
  if (myCard.mode == 2) {
    Serial.println(F("Albummodus ist aktiv -> vorheriger Track"));
    if (currentTrack != 1) {
      currentTrack = currentTrack - 1;
    }
    mp3.playFolderTrack(myCard.folder, currentTrack);
  }
  if (myCard.mode == 3) {
    Serial.println(F("Party Modus ist aktiv -> Track von vorne spielen"));
    mp3.playFolderTrack(myCard.folder, currentTrack);
  }
  if (myCard.mode == 4) {
    Serial.println(F("Einzel Modus aktiv -> Track von vorne spielen"));
    mp3.playFolderTrack(myCard.folder, currentTrack);
  }
  if (myCard.mode == 5) {
    Serial.println(F("Hörbuch Modus ist aktiv -> vorheriger Track und "
                     "Fortschritt speichern"));
    if (currentTrack != 1) {
      currentTrack = currentTrack - 1;
    }
    mp3.playFolderTrack(myCard.folder, currentTrack);
    // Fortschritt im EEPROM abspeichern
    EEPROM.write(myCard.folder, currentTrack);
  }
}

// MFRC522
#define RST_PIN 9                 // Configurable, see typical pin layout above
#define SS_PIN 10                 // Configurable, see typical pin layout above
MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522
MFRC522::MIFARE_Key key;
bool successRead;
byte sector = 1;
byte blockAddr = 4;
byte trailerBlock = 7;
MFRC522::StatusCode status;

#define buttonPause A0
#define buttonUp A1
#define buttonDown A2
#define buttonVolUp A3         //Zusätzlicher Knopf für Lautstärke hoch
#define buttonVolDown A4       //Zusätzlicher Knopf für Lautstärke runter
#define busyPin 4
#define LONG_PRESS 3000

Button pauseButton(buttonPause);
Button upButton(buttonUp);
Button downButton(buttonDown);
Button VolUpButton(buttonVolUp);         //Knopf für Lautstärke hoch
Button VolDownButton(buttonVolDown);     //Knopf für Lautstärke runter
bool ignorePauseButton = false;
bool ignoreUpButton = false;
bool ignoreDownButton = false;

uint8_t numberOfCards = 0;

bool isPlaying() {
  return !digitalRead(busyPin);
}

void setup() {
  // Adafruit Animation zum Start
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  strip.setBrightness(STARTBRIGHTNESS);
  rainbowCycle(2); //RainbowCycle von Adafruit abspielen
  colorWipe(strip.Color(0, 0, 0), 0); // AUS
  strip.show();
  
  // FastLED
  // delay(3000); // 3 second delay for recovery
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip); // tell FastLED about the LED strip configuration
  FastLED.setBrightness(BRIGHTNESS1);
  fill_solid(leds, NUM_LEDS, CRGB::Purple);
  FastLED.show();

  Serial.begin(115200); // Es gibt ein paar Debug Ausgaben über die serielle Schnittstelle
  randomSeed(analogRead(A0)); // Zufallsgenerator initialisieren

  Serial.println(F("TonUINO Version 2.0"));
  Serial.println(F("(c) Thorsten Voß"));
  Serial.println(F("LED Ring + 5 Tasten MOD"));  

  // Knöpfe mit PullUp
  pinMode(buttonPause, INPUT_PULLUP);
  pinMode(buttonUp, INPUT_PULLUP);
  pinMode(buttonDown, INPUT_PULLUP);
  pinMode(buttonVolUp, INPUT_PULLUP);         //Zusätzliche Buttons Vol Up
  pinMode(buttonVolDown, INPUT_PULLUP);       //Zusätzliche Buttons Vol down

  // Busy Pin
  pinMode(busyPin, INPUT);

  // DFPlayer Mini initialisieren
  mp3.begin();
  mp3.setVolume(12); // Startlautstärke
  volume = 12;
  minVolume = 1; //  minimale Lautstärke
  maxVolume = 25; // maximale Lautatärke
  Serial.println(F("Lautstärke:" ));
  Serial.println(volume);
  
  // NFC Leser initialisieren
  SPI.begin();        // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522
  mfrc522
  .PCD_DumpVersionToSerial(); // Show details of PCD - MFRC522 Card Reader
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  // RESET --- ALLE DREI KNÖPFE BEIM STARTEN GEDRÜCKT HALTEN -> alle bekannten
  // Karten werden gelöscht
  if (digitalRead(buttonPause) == LOW && digitalRead(buttonUp) == LOW &&
      digitalRead(buttonDown) == LOW) {
    Serial.println(F("Reset -> EEPROM wird gelöscht"));
    for (int i = 0; i < EEPROM.length(); i++) {
      EEPROM.write(i, 0);
    }
  }
//  mp3.playMp3FolderTrack(998); //Startmelodie muss im mp3 Ordner liegen und so beginnen 998_blaxxxbla.mp3
}

void loop(){
  do {
    //Pride Animation. Super!
     pride();
     FastLED.show(); 
    // FastLED
    if (isPlaying()) {    // Prüfung, ob MP3 abgespielt wird, wenn ja, dann jeweilige LED-Animation abspielen
      FastLED.setBrightness(BRIGHTNESS2); // Helligkeit 2 während Animation beim Abspielen
      
      if (myCard.mode == 1) { // Hörspielmodus -> zufälligen Track wiedergeben
        rainbowWithGlitter(); // FastLED's rainbowWithGlitter ausführen
      }
      if (myCard.mode == 2) { // Album Modus -> kompletten Ordner wiedergeben
        rainbow();          // FastLED's  Regenbogen ausführen
      }
      if (myCard.mode == 3) { // Party Modus -> Ordner in zufälliger Reihenfolge
        confetti();           // FastLED's  confetti ausführen
      }      
      if (myCard.mode == 4) { // Einzel Modus: eine Datei aus dem Ordner abspielen
        sinelon();        // FastLED's  sinelon ausführen
      }           
      if (myCard.mode == 5) { // Hörbuch Modus: kompletten Ordner spielen und Fortschritt merken
        rainbow();            // FastLED's  Regenbogen ausführen
      }           
      FastLED.show(); // send the 'leds' array out to the actual LED strip
      FastLED.delay(1000 / FRAMES_PER_SECOND); // insert a delay to keep the framerate modest
      EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
    } 
    
    mp3.loop();
    // Buttons werden nun über JS_Button gehandelt, dadurch kann jede Taste doppelt belegt werden
    pauseButton.read();
    upButton.read();
    downButton.read();
    VolUpButton.read();        //Zusätzliche Buttons 
    VolDownButton.read();      //Zusätzliche Buttons

    if (pauseButton.wasReleased()) {
      if (ignorePauseButton == false)
        if (isPlaying()) {
          mp3.pause();
          // FastLED.clear (); // alle LEDs ausschalten
          // fill_rainbow( leds, NUM_LEDS, 0, 5); // Rainbow Füllung
          FastLED.setBrightness(BRIGHTNESS1);
          fill_solid(leds, NUM_LEDS, CRGB::Purple);
          FastLED.show(); // Befehl zum LED Ring senden
        }
        else {
          mp3.start();
        }
      ignorePauseButton = false;
    } else if (pauseButton.pressedFor(LONG_PRESS) &&
               ignorePauseButton == false) {
      if (isPlaying())
        mp3.playAdvertisement(currentTrack);
      else {
        knownCard = false;
        mp3.playMp3FolderTrack(800);
        Serial.println(F("Karte resetten..."));
        resetCard();
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
      }
      ignorePauseButton = true;
    }

  if (VolUpButton.wasReleased()) {
    if (volume < maxVolume) {
      Serial.println(F("Volume Up"));
      mp3.increaseVolume();        // Laustärke erhohen
        volume = mp3.getVolume(); // aktuelle Lautstärke abfragen und in Variable schreiben
        Serial.println(volume);
      FastLED.setBrightness(BRIGHTNESS3);
      fill_solid(leds, NUM_LEDS, CRGB::Red);  // Red zeigt Grün an
      FastLED.show();
      FastLED.delay(500);
      FastLED.clear ();         // alle LEDs ausschalten (falls im Pause Modus
      FastLED.setBrightness(BRIGHTNESS1);
      //fill_solid(leds, NUM_LEDS, CRGB::Purple);
      FastLED.show(); 
        }
     }
    
  if (upButton.wasReleased()) {      
    nextTrack(random(65536));                               
  }
  
  if (VolDownButton.wasReleased()) {
    if (volume > minVolume) {
    Serial.println(F("Volume Down"));
    mp3.decreaseVolume();       // Laustärke verringern
      volume = mp3.getVolume(); // aktuelle Lautstärke abfragen und in Variable schreiben
      Serial.println(volume);
    FastLED.setBrightness(BRIGHTNESS3);
    fill_solid(leds, NUM_LEDS, CRGB::Green);    //Green zeigt Rot an
    FastLED.show();
    FastLED.delay(500);
    // FastLED.clear ();        // alle LEDs ausschalten falls im Pause Modus
    FastLED.setBrightness(BRIGHTNESS1);
    //fill_solid(leds, NUM_LEDS, CRGB::Purple);
    FastLED.show();
    ignoreDownButton = true;
      }                      
  }
   
  if (downButton.wasReleased()) {
    previousTrack();                         
  }
    
// Ende der Buttons
  
  } while (!mfrc522.PICC_IsNewCardPresent());

  // RFID Karte wurde aufgelegt

  if (!mfrc522.PICC_ReadCardSerial())
    return;

  if (readCard(&myCard) == true) {
    if (myCard.cookie == 322417479 && myCard.folder != 0 && myCard.mode != 0) {

      knownCard = true;
      _lastTrackFinished = 0;
      numTracksInFolder = mp3.getFolderTrackCount(myCard.folder);
      Serial.print(numTracksInFolder);
      Serial.print(F(" Dateien in Ordner "));
      Serial.println(myCard.folder);

      // Hörspielmodus: eine zufällige Datei aus dem Ordner
      if (myCard.mode == 1) {
        Serial.println(F("Hörspielmodus -> zufälligen Track wiedergeben"));
        currentTrack = random(1, numTracksInFolder + 1);
        Serial.println(currentTrack);
        mp3.playFolderTrack(myCard.folder, currentTrack);
      }
      // Album Modus: kompletten Ordner spielen
      if (myCard.mode == 2) {
        Serial.println(F("Album Modus -> kompletten Ordner wiedergeben"));
        currentTrack = 1;
        mp3.playFolderTrack(myCard.folder, currentTrack);
      }
      // Party Modus: Ordner in zufälliger Reihenfolge
      if (myCard.mode == 3) {
        Serial.println(F("Party Modus -> Ordner in zufälliger Reihenfolge wiedergeben"));
        currentTrack = random(1, numTracksInFolder + 1);
        mp3.playFolderTrack(myCard.folder, currentTrack);
      }
      // Einzel Modus: eine Datei aus dem Ordner abspielen
      if (myCard.mode == 4) {
        Serial.println(
          F("Einzel Modus -> eine Datei aus dem Odrdner abspielen"));
        currentTrack = myCard.special;
        mp3.playFolderTrack(myCard.folder, currentTrack);
      }
      // Hörbuch Modus: kompletten Ordner spielen und Fortschritt merken
      if (myCard.mode == 5) {
        Serial.println(F("Hörbuch Modus -> kompletten Ordner spielen und "
                         "Fortschritt merken"));
        currentTrack = EEPROM.read(myCard.folder);
        mp3.playFolderTrack(myCard.folder, currentTrack);
      }
    }

    // Neue Karte konfigurieren
    else {
      knownCard = false;
      setupCard();
    }
  }
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

int voiceMenu(int numberOfOptions, int startMessage, int messageOffset,
              bool preview = false, int previewFromFolder = 0) {
  int returnValue = 0;
  if (startMessage != 0)
    mp3.playMp3FolderTrack(startMessage);
  do {
    pauseButton.read();
    upButton.read();
    downButton.read();
    mp3.loop();
    if (pauseButton.wasPressed()) {
      if (returnValue != 0)
        return returnValue;
      delay(1000);
    }

    if (upButton.pressedFor(LONG_PRESS)) {
      returnValue = min(returnValue + 10, numberOfOptions);
      mp3.playMp3FolderTrack(messageOffset + returnValue);
      delay(1000);
      if (preview) {
        do {
          delay(10);
        } while (isPlaying());
        if (previewFromFolder == 0)
          mp3.playFolderTrack(returnValue, 1);
        else
          mp3.playFolderTrack(previewFromFolder, returnValue);
      }
      ignoreUpButton = true;
    } else if (upButton.wasReleased()) {
      if (!ignoreUpButton) {
        returnValue = min(returnValue + 1, numberOfOptions);
        mp3.playMp3FolderTrack(messageOffset + returnValue);
        delay(1000);
        if (preview) {
          do {
            delay(10);
          } while (isPlaying());
          if (previewFromFolder == 0)
            mp3.playFolderTrack(returnValue, 1);
          else
            mp3.playFolderTrack(previewFromFolder, returnValue);
        }
      } else
        ignoreUpButton = false;
    }

    if (downButton.pressedFor(LONG_PRESS)) {
      returnValue = max(returnValue - 10, 1);
      mp3.playMp3FolderTrack(messageOffset + returnValue);
      delay(1000);
      if (preview) {
        do {
          delay(10);
        } while (isPlaying());
        if (previewFromFolder == 0)
          mp3.playFolderTrack(returnValue, 1);
        else
          mp3.playFolderTrack(previewFromFolder, returnValue);
      }
      ignoreDownButton = true;
    } else if (downButton.wasReleased()) {
      if (!ignoreDownButton) {
        returnValue = max(returnValue - 1, 1);
        mp3.playMp3FolderTrack(messageOffset + returnValue);
        delay(1000);
        if (preview) {
          do {
            delay(10);
          } while (isPlaying());
          if (previewFromFolder == 0)
            mp3.playFolderTrack(returnValue, 1);
          else
            mp3.playFolderTrack(previewFromFolder, returnValue);
        }
      } else
        ignoreDownButton = false;
    }
  } while (true);
}

void resetCard() {
  do {
    pauseButton.read();
    upButton.read();
    downButton.read();

    if (upButton.wasReleased() || downButton.wasReleased()) {
      Serial.print(F("Abgebrochen!"));
      mp3.playMp3FolderTrack(802);
      colorWipe(strip.Color(0, 255, 0), 30); // Wipe in Rot
      strip.show();
      colorWipe(strip.Color(0, 0, 0), 0); // AUS
      strip.show();
      delay(3000);
      FastLED.setBrightness(BRIGHTNESS1);
      fill_solid(leds, NUM_LEDS, CRGB::Purple);
      FastLED.show();
      return;
    }
  } while (!mfrc522.PICC_IsNewCardPresent());

  if (!mfrc522.PICC_ReadCardSerial())
    return;
  Serial.print(F("Karte wird neu Konfiguriert!"));
  setupCard();
}

void setupCard() {
  mp3.pause();
  Serial.print(F("Neue Karte konfigurieren"));

  // Ordner abfragen
  myCard.folder = voiceMenu(99, 300, 0, true);
  blink_green_one();
  FastLED.setBrightness(BRIGHTNESS1);
  fill_solid(leds, NUM_LEDS, CRGB::Purple);
  FastLED.show();
  
  // Wiedergabemodus abfragen
  myCard.mode = voiceMenu(6, 310, 310);

  // Hörbuchmodus -> Fortschritt im EEPROM auf 1 setzen
  EEPROM.write(myCard.folder, 1);

  // Einzelmodus -> Datei abfragen
  if (myCard.mode == 4)
    myCard.special = voiceMenu(mp3.getFolderTrackCount(myCard.folder), 320, 0,
                               true, myCard.folder);

  // Admin Funktionen
  if (myCard.mode == 6)
    myCard.special = voiceMenu(3, 320, 320);

  // Karte ist konfiguriert -> speichern
  mp3.pause();
  writeCard(myCard);
}

bool readCard(nfcTagObject *nfcTag) {
  bool returnValue = true;
  // Show some details of the PICC (that is: the tag/card)
  Serial.print(F("Card UID:"));
  dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
  Serial.println();
  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.println(mfrc522.PICC_GetTypeName(piccType));

  byte buffer[18];
  byte size = sizeof(buffer);

  // Authenticate using key A
  Serial.println(F("Authenticating using key A..."));
  status = (MFRC522::StatusCode)mfrc522.PCD_Authenticate(
             MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    returnValue = false;
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  // Show the whole sector as it currently is
  Serial.println(F("Current data in sector:"));
  mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
  Serial.println();

  // Read data from the block
  Serial.print(F("Reading data from block "));
  Serial.print(blockAddr);
  Serial.println(F(" ..."));
  status = (MFRC522::StatusCode)mfrc522.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    returnValue = false;
    Serial.print(F("MIFARE_Read() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
  }
  Serial.print(F("Data in block "));
  Serial.print(blockAddr);
  Serial.println(F(":"));
  dump_byte_array(buffer, 16);
  Serial.println();
  Serial.println();

  uint32_t tempCookie;
  tempCookie = (uint32_t)buffer[0] << 24;
  tempCookie += (uint32_t)buffer[1] << 16;
  tempCookie += (uint32_t)buffer[2] << 8;
  tempCookie += (uint32_t)buffer[3];

  nfcTag->cookie = tempCookie;
  nfcTag->version = buffer[4];
  nfcTag->folder = buffer[5];
  nfcTag->mode = buffer[6];
  nfcTag->special = buffer[7];

  return returnValue;
}


// LED-Part von Adafruit-Library
void colorWipe(uint32_t c, uint8_t wait) { // Adafruit´s - Fill the dots one after the other with a color
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}
void rainbowCycle(uint8_t wait) { // Adafruit´s Rainbow Cycle
  uint16_t i, j;
  for (j = 0; j < 256 * 2; j++) { // 1 cycles of all colors on wheel
    for (i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}
// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}


// LED-Part von FastLED-Library 
void rainbow() // FastLED's built-in rainbow generator
{
  fill_rainbow( leds, NUM_LEDS, gHue, 7); 
}
void rainbowWithGlitter()   // built-in FastLED rainbow, plus some random sparkly glitter
{
  rainbow();
  addGlitter(80);
}
void addGlitter( fract8 chanceOfGlitter)
{
  if ( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}
void confetti()   // random colored speckles that blink in and fade smoothly
{
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
}
void sinelon()   // a colored dot sweeping back and forth, with fading trails
{
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16( 13, 0, NUM_LEDS-1 );
  leds[pos] += CHSV( gHue, 255, 192);
}
void blink_green_one()
{
  FastLED.setBrightness(BRIGHTNESS3);
  fill_solid(leds, NUM_LEDS, CRGB::Green);
  FastLED.show();
  FastLED.delay(500);
  delay(130);
  FastLED.clear (); // alle LEDs ausschalten
  FastLED.show();
}

// PRIDE This function draws rainbows with an ever-changing,
// widely-varying set of parameters.
void pride() 
{
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;
 
  uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 1, 3000);
  
  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88( 400, 5,9);
  uint16_t brightnesstheta16 = sPseudotime;
  
  for( uint16_t i = 0 ; i < NUM_LEDS; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);
    
    CRGB newcolor = CHSV( hue8, sat8, bri8);
    
    uint16_t pixelnumber = i;
    pixelnumber = (NUM_LEDS-1) - pixelnumber;
    
    nblend( leds[pixelnumber], newcolor, 64);
  }
}

// TonUINO Part
void writeCard(nfcTagObject nfcTag) {
  MFRC522::PICC_Type mifareType;
  byte buffer[16] = {0x13, 0x37, 0xb3, 0x47, // 0x1337 0xb347 magic cookie to
                     // identify our nfc tags
                     0x01,                   // version 1
                     nfcTag.folder,          // the folder picked by the user
                     nfcTag.mode,    // the playback mode picked by the user
                     nfcTag.special, // track or function for admin cards
                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                    };

  byte size = sizeof(buffer);

  mifareType = mfrc522.PICC_GetType(mfrc522.uid.sak);

  // Authenticate using key B
  Serial.println(F("Authenticating again using key B..."));
  status = (MFRC522::StatusCode)mfrc522.PCD_Authenticate(
             MFRC522::PICC_CMD_MF_AUTH_KEY_B, trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    mp3.playMp3FolderTrack(401);
    return;
  }

  // Write data to the block
  Serial.print(F("Writing data into block "));
  Serial.print(blockAddr);
  Serial.println(F(" ..."));
  dump_byte_array(buffer, 16);
  Serial.println();
  status = (MFRC522::StatusCode)mfrc522.MIFARE_Write(blockAddr, buffer, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    mp3.playMp3FolderTrack(401);
    FastLED.setBrightness(BRIGHTNESS3);
    colorWipe(strip.Color(0, 255, 0), 30); // Wipe in Rot
    strip.show();
    colorWipe(strip.Color(0, 0, 0), 0); // AUS
    strip.show();
    delay(3200);
    FastLED.setBrightness(BRIGHTNESS1);
    fill_solid(leds, NUM_LEDS, CRGB::Purple);
    FastLED.show();
  }
  else {
    mp3.playMp3FolderTrack(400); // MP3 Ansage dass die Karte konf ist
    FastLED.setBrightness(BRIGHTNESS3);
    colorWipe(strip.Color(255, 0, 0), 30); // Wipe in Gruen
//    colorWipe(strip.Color(0, 0, 255), 30); // Wipe in Blau
    strip.show();
    colorWipe(strip.Color(0, 0, 0), 0); // AUS
    strip.show();
    delay(3200);
    FastLED.setBrightness(BRIGHTNESS1);
    fill_solid(leds, NUM_LEDS, CRGB::Purple);
    FastLED.show();
  }
  Serial.println();
  delay(100);
}

/**
   Helper routine to dump a byte array as hex values to Serial.
*/
void dump_byte_array(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}
