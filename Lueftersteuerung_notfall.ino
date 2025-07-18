#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <EEPROM.h>

// Definition der Pins und Typen
#define DHTPIN_INSIDE 5    // Pin für den Innensensor DHT22
#define DHTPIN_OUTSIDE 6   // Pin für den Außensensor DHT22
#define DHTTYPE DHT22      // DHT22 (AM2302) Sensortyp
#define RELAY_PIN 7        // Pin für das Relais zur Lüftersteuerung
#define ENCODER_PIN_A 2    // Rotary Encoder Pin A (CLK)
#define ENCODER_PIN_B 3    // Rotary Encoder Pin B (DT)
#define ENCODER_SW 4       // Rotary Encoder Switch

// Definition für die invertierte Logik des Lüfterrelais
#define LUEFTERHIGH 0      // Lüfter an (LOW bei invertierter Logik)
#define LUEFTERLOW 1       // Lüfter aus (HIGH bei invertierter Logik)

// PROGMEM für Menüstrings nutzen, um RAM zu sparen
const char str_menu[] PROGMEM = "MENU:";
const char str_modus[] PROGMEM = "MODUS:";
const char str_luefter[] PROGMEM = "LUEFTER MANUELL:";
const char str_zieltemp[] PROGMEM = "ZIELTEMPERATUR:";
const char str_ziellf[] PROGMEM = "ZIEL LF:";
const char str_mintemp[] PROGMEM = "MIN TEMPERATUR:";
const char str_kalibrieren[] PROGMEM = "KALIBRIEREN:";
const char str_kalib_innen[] PROGMEM = "KALIBR. INNEN:";
const char str_kalib_aussen[] PROGMEM = "KALIBR. AUSSEN:";
const char str_temp_innen_kal[] PROGMEM = "TEMP INNEN KAL:";
const char str_lf_innen_kal[] PROGMEM = "LF INNEN KAL:";
const char str_temp_aussen_kal[] PROGMEM = "TEMP AUSSEN KAL:";
const char str_lf_aussen_kal[] PROGMEM = "LF AUSSEN KAL:";

// Initialisierung der Komponenten
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht_inside(DHTPIN_INSIDE, DHTTYPE);
DHT dht_outside(DHTPIN_OUTSIDE, DHTTYPE);

// Globale Variablen für den Rotary Encoder
volatile int lastEncoded = 0;
volatile int stepCounter = 0;
int timer= 0;

// Lüftermodi
enum FanMode {AUTO, MANUAL_ON, MANUAL_OFF};
FanMode fanMode = AUTO;


// Schwellenwerte
float targetTemp = 20.0;
float targetHum = 55.0;
float minTemp = 5.0;
float tempOffsetInsideTemp = 0.0;
float tempOffsetInsideHum = 0.0;
float tempOffsetOutsideTemp = 0.0;
float tempOffsetOutsideHum = 0.0;

// EEPROM Offsets
const byte EEPROM_ADDRESS = 0;

// Hilfsfunktion zum Laden von Strings aus PROGMEM
void loadAndPrintProgmemStr(const char* str) {
  char c;
  while ((c = pgm_read_byte(str++))) {
    lcd.print(c);
  }
}

// Funktion zur Initialisierung der Hardware
void setup() {
  // Setup der Pins
  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);
  pinMode(ENCODER_SW, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  
  // Test-Sequenz für Relais
  digitalWrite(RELAY_PIN, LUEFTERHIGH);
  delay(500);
  digitalWrite(RELAY_PIN, LUEFTERLOW);
  
  // Rotary Encoder Interrupts
/*  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), updateEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), updateEncoder, CHANGE);*/
  
  lcd.begin(16, 2);
  lcd.backlight(); // LCD-Hintergrundbeleuchtung einschalten
  
  // Begrüßung und Relais-Test anzeigen
  lcd.clear();

  
  dht_inside.begin();
  dht_outside.begin();

/*EEPROM.write(EEPROM_ADDRESS, 123); // Markierung für Initialisierung setzen
  EEPROM.put(1, 15.0);
  EEPROM.put(5, 55.0);
  EEPROM.put(9, 5.0);
  EEPROM.put(13, 0.0);
  EEPROM.put(17, 0.0);
  EEPROM.put(21, 0.0);
  EEPROM.put(25, 0.0);
 */ 
 // loadSettings();  // Lade gespeicherte Werte aus dem EEPROM
  }

// Hauptschleife
void loop() {
  static unsigned long lastDebounceTime = 0;
  const unsigned long debounceDelay = 200; // 200ms Entprellung
  static unsigned long lastStatusTime = 0;

      // Nur alle 2 Sekunden aktualisieren, um Flackern zu vermeiden
      if (millis() - lastStatusTime > 2000) {
        lastStatusTime = millis();
        displayStatus();  // Status anzeigen
        controlFan();     // Lüfter steuern
      }

  delay(50); // Reduzierte Verzögerung für bessere Reaktionsfähigkeit
}

// Funktion zur Steuerung des Lüfters basierend auf den Sensormessungen und Modi
void controlFan() {
  // Temperatur und Feuchtigkeit der Sensoren messen
  float insideTemp = dht_inside.readTemperature() + tempOffsetInsideTemp;
  float outsideTemp = dht_outside.readTemperature() + tempOffsetOutsideTemp;
  float insideHum = dht_inside.readHumidity() + tempOffsetInsideHum;
  float outsideHum = dht_outside.readHumidity() + tempOffsetOutsideHum;
  
  // Wenn eine Sensorlesung fehlschlägt, schalte den Lüfter aus
  if (isnan(insideTemp) || isnan(outsideTemp) || isnan(insideHum) || isnan(outsideHum)) {
    digitalWrite(RELAY_PIN, LUEFTERLOW);
        return;
  }
  
  // Steuerlogik für den Sommermodus
  
   bool shouldTurnFanOn = 0;

    if((insideTemp > outsideTemp && outsideHum < insideHum) ||(insideHum > targetHum && insideHum <(outsideHum*0.8))){
      shouldTurnFanOn = 1;
    }     
      if(timer > 10)
      {  lcd.clear();

        digitalWrite(RELAY_PIN, shouldTurnFanOn ? LUEFTERHIGH : LUEFTERLOW);
      timer = 0;
        lcd.clear();
        displayStatus();

      
      }else{
      timer ++;  
    }

}

// Funktion zur Anzeige des aktuellen Status auf dem LCD
void displayStatus() {
  float insideTemp = dht_inside.readTemperature() + tempOffsetInsideTemp;
  float outsideTemp = dht_outside.readTemperature() + tempOffsetOutsideTemp;
  float insideHum = dht_inside.readHumidity() + tempOffsetInsideHum;
  float outsideHum = dht_outside.readHumidity() + tempOffsetOutsideHum;
  
  lcd.clear();
  // Erste Zeile: Temperatur und Anzeige des Modus (S/W)
  lcd.setCursor(0, 0);
  lcd.print("S "); 
  
  lcd.print("I:"); // Temperatur Innen
  if (isnan(insideTemp)) {
    lcd.print("XX.X"); // Fehleranzeige
  } else {
    lcd.print(insideTemp, 1);
  }
  lcd.print(" A:"); // Temperatur Außen
  if (isnan(outsideTemp)) {
    lcd.print("XX.X");
  } else {
    lcd.print(outsideTemp, 1);
  }
  
  lcd.setCursor(0, 1);
  lcd.print("LF:I:"); // Luftfeuchtigkeit Innen
  if (isnan(insideHum)) {
    lcd.print("XX.X");
  } else {
    lcd.print(insideHum, 1);
  }
  lcd.print(" A:"); // Luftfeuchtigkeit Außen
  if (isnan(outsideHum)) {
    lcd.print("XX.X");
  } else {
    lcd.print(outsideHum, 1);
  }
}
