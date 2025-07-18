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


// Initialisierung der Komponenten
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht_inside(DHTPIN_INSIDE, DHTTYPE);
DHT dht_outside(DHTPIN_OUTSIDE, DHTTYPE);

// Globale Variablen für den Rotary Encoder
volatile int lastEncoded = 0;
volatile int stepCounter = 0;
int timer = 0;

// Betriebsmodi
enum Mode {SUMMER, WINTER};
Mode mode = SUMMER;
Mode previousMode = SUMMER; // Speichern des vorherigen Modus

// Menüpunkte
enum Menu {MAIN, MODE_SELECTION, FAN_MANUAL, TARGET_TEMP, TARGET_HUM, MIN_TEMP,
           CALIBRATE_SENSOR, CALIBRATE_INSIDE, CALIBRATE_OUTSIDE, CALIBRATE_TEMP_INSIDE,
           CALIBRATE_HUMIDITY_INSIDE, CALIBRATE_TEMP_OUTSIDE, CALIBRATE_HUMIDITY_OUTSIDE
          };
Menu currentMenu = MAIN;
byte menuIndex = 0;
bool inMenu = false;  // Zeigt an, ob wir im Menü sind
bool editMode = false;

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
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), updateEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), updateEncoder, CHANGE);

  lcd.begin(16, 2);
  lcd.backlight(); // LCD-Hintergrundbeleuchtung einschalten

  // Begrüßung und Relais-Test anzeigen
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Luefter-Steuerung");
  lcd.setCursor(0, 1);
  lcd.print("Relais-Test...");

  dht_inside.begin();
  dht_outside.begin();
  loadSettings();  // Lade gespeicherte Werte aus dem EEPROM

  delay(1000); // Kurze Verzögerung, damit Begrüßung sichtbar bleibt
}

// Hauptschleife
void loop() {
  static unsigned long lastDebounceTime = 0;
  const unsigned long debounceDelay = 200; // 200ms Entprellung
  static unsigned long lastStatusTime = 0;

  // Prüfe, ob sich der Modus geändert hat
  if (mode != previousMode) {
    previousMode = mode;
    / TODO nötig ?
  }

  // Encoder-Button-Überprüfung mit Entprellung
  if (digitalRead(ENCODER_SW) == LOW) {
    if ((millis() - lastDebounceTime) > debounceDelay) {
      lastDebounceTime = millis();

      if (!inMenu) {
        inMenu = true;  // Menü öffnen
        menuIndex = 0;
      } else if (editMode) {
        // Beim Verlassen des Editiermodus Einstellungen speichern
        editMode = false;
        saveSettings();
      } else {
        handleMenuSelect(); // Menüpunkte verwalten
      }
    }
  }

  // Anzeigensteuerung
  if (!inMenu) {

    // Nur alle 2 Sekunden aktualisieren, um Flackern zu vermeiden
    if (millis() - lastStatusTime > 2000) {
      lastStatusTime = millis();
      displayStatus();  // Status anzeigen
      controlFan();     // Lüfter steuern
    }
  }
} else {
  // Nur aktualisieren, wenn sich Menü oder Index geändert haben
  static Menu lastMenu = currentMenu;
  static int lastIndex = menuIndex;
  static bool lastEditMode = editMode;

  if (lastMenu != currentMenu || lastIndex != menuIndex || lastEditMode != editMode) {
    displayMenu(); // Menü anzeigen
    lastMenu = currentMenu;
    lastIndex = menuIndex;
    lastEditMode = editMode;
  }
}

delay(50); // Reduzierte Verzögerung für bessere Reaktionsfähigkeit
}

// Funktion zur Steuerung des Lüfters basierend auf den Sensormessungen und Modi
void controlFan() {
  bool shouldTurnFanOn = 0;

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
  if (mode == SUMMER) {

    if ((insideTemp > outsideTemp && outsideHum < insideHum) || (insideHum > targetHum && insideHum < (outsideHum * 0.8))) {
      shouldTurnFanOn = 1;
    }
  }

  // Steuerlogik für den Wintermodus
  if (mode == WINTER) {
    if ((outsideTemp > minTemp && insideHum > targetHum && outsideHum < insideHum) || (outsideTemp > insideTemp)) {
      shouldTurnFanOn = 1;
    }
  }

  //gesetzten Zustand umsetzen
  if (shouldTurnFanOn == 1) {
    if (timer > 20)
    { digitalWrite(RELAY_PIN, LUEFTERHIGH);
      timer = 0;
    }
    timer ++;
  } else {
    digitalWrite(RELAY_PIN, LUEFTERLOW);
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
  lcd.print(mode == SUMMER ? "S " : "W ");

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



// Funktion zur Darstellung und Navigation durch das Menü
void displayMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);

  switch (currentMenu) {
    case MAIN:
      loadAndPrintProgmemStr(str_menu);
      lcd.setCursor(0, 1);
      switch (menuIndex) {
        case 0: lcd.print(">Modus"); break;
        case 1: lcd.print(">Luefter manuell"); break;
        case 2: lcd.print(">Ziel Temp."); break;
        case 3: lcd.print(">Ziel Luftfeuchte"); break;
        case 4: lcd.print(">Min. Temp."); break;
        case 5: lcd.print(">Kalibriere Sens"); break;
        case 6: lcd.print(">Zurueck"); break;
      }
      break;

    case MODE_SELECTION:
      loadAndPrintProgmemStr(str_modus);
      lcd.setCursor(0, 1);
      switch (menuIndex) {
        case 0: lcd.print(">Sommer"); break;
        case 1: lcd.print(">Winter"); break;
        case 2: lcd.print(">Zurueck"); break;
      }
      break;

    case TARGET_TEMP:
      loadAndPrintProgmemStr(str_zieltemp);
      displayEditOrShow(targetTemp, " C");
      break;

    case TARGET_HUM:
      loadAndPrintProgmemStr(str_ziellf);
      displayEditOrShow(targetHum, " %");
      break;

    case MIN_TEMP:
      loadAndPrintProgmemStr(str_mintemp);
      displayEditOrShow(minTemp, " C");
      break;

    case CALIBRATE_SENSOR:
      loadAndPrintProgmemStr(str_kalibrieren);
      lcd.setCursor(0, 1);
      switch (menuIndex) {
        case 0: lcd.print(">Innen"); break;
        case 1: lcd.print(">Aussen"); break;
        case 2: lcd.print(">Zurueck"); break;
      }
      break;

    // Weitere Kalibriermenüs
    case CALIBRATE_INSIDE:
      loadAndPrintProgmemStr(str_kalib_innen);
      lcd.setCursor(0, 1);
      switch (menuIndex) {
        case 0: lcd.print(">Temperatur"); break;
        case 1: lcd.print(">Luftfeuchtigkeit"); break;
        case 2: lcd.print(">Zurueck"); break;
      }
      break;

    case CALIBRATE_OUTSIDE:
      loadAndPrintProgmemStr(str_kalib_aussen);
      lcd.setCursor(0, 1);
      switch (menuIndex) {
        case 0: lcd.print(">Temperatur"); break;
        case 1: lcd.print(">Luftfeuchtigkeit"); break;
        case 2: lcd.print(">Zurueck"); break;
      }
      break;

    case CALIBRATE_TEMP_INSIDE:
      loadAndPrintProgmemStr(str_temp_innen_kal);
      displayEditOrShow(tempOffsetInsideTemp, " C");
      break;

    case CALIBRATE_HUMIDITY_INSIDE:
      loadAndPrintProgmemStr(str_lf_innen_kal);
      displayEditOrShow(tempOffsetInsideHum, " %");
      break;

    case CALIBRATE_TEMP_OUTSIDE:
      loadAndPrintProgmemStr(str_temp_aussen_kal);
      displayEditOrShow(tempOffsetOutsideTemp, " C");
      break;

    case CALIBRATE_HUMIDITY_OUTSIDE:
      loadAndPrintProgmemStr(str_lf_aussen_kal);
      displayEditOrShow(tempOffsetOutsideHum, " %");
      break;
  }
}

// Funktion zur Verarbeitung der Menünavigation und Auswahl
void handleMenuSelect() {
  switch (currentMenu) {
    case MAIN:
      // Auswahl von Menüpunkten im Hauptmenü
      if (menuIndex == 0) setCurrentMenu(MODE_SELECTION);
      else if (menuIndex == 1) setCurrentMenu(FAN_MANUAL);
      else if (menuIndex == 2) {
        setCurrentMenu(TARGET_TEMP);
        editMode = true;
      }
      else if (menuIndex == 3) {
        setCurrentMenu(TARGET_HUM);
        editMode = true;
      }
      else if (menuIndex == 4) {
        setCurrentMenu(MIN_TEMP);
        editMode = true;
      }
      else if (menuIndex == 5) setCurrentMenu(CALIBRATE_SENSOR);
      else if (menuIndex == 6) {
        inMenu = false; // Zurück aus dem Menü
      }
      break;

    case MODE_SELECTION:
      if (menuIndex == 0) {
        mode = SUMMER;
        setCurrentMenu(MAIN); // Zurück zum Hauptmenü
      }
      else if (menuIndex == 1) {
        mode = WINTER;
        setCurrentMenu(MAIN); // Zurück zum Hauptmenü
      }
      else if (menuIndex == 2) {
        setCurrentMenu(MAIN); // Zurück zum Hauptmenü
      }
      break;

    case TARGET_TEMP:
    case TARGET_HUM:
    case MIN_TEMP:
      // Wenn wir nicht im Editiermodus sind, zum Hauptmenü zurückkehren
      if (!editMode) {
        setCurrentMenu(MAIN);
      } else {
        // Im Editiermodus bleiben - der Encoder ändert die Werte
        // Keine Aktion hier notwendig, da wir den Editiermodus beim Knopfdruck verlassen
      }
      break;

    case CALIBRATE_TEMP_INSIDE:
    case CALIBRATE_HUMIDITY_INSIDE:
      if (!editMode) {
        setCurrentMenu(CALIBRATE_INSIDE);
      }
      break;

    case CALIBRATE_TEMP_OUTSIDE:
    case CALIBRATE_HUMIDITY_OUTSIDE:
      if (!editMode) {
        setCurrentMenu(CALIBRATE_OUTSIDE);
      }
      break;

    case CALIBRATE_SENSOR:
      if (menuIndex == 0) setCurrentMenu(CALIBRATE_INSIDE);
      else if (menuIndex == 1) setCurrentMenu(CALIBRATE_OUTSIDE);
      else if (menuIndex == 2) setCurrentMenu(MAIN); // Zurück zum Hauptmenü
      break;

    case CALIBRATE_INSIDE:
      if (menuIndex == 0) {
        setCurrentMenu(CALIBRATE_TEMP_INSIDE);
        editMode = true; // Editiermodus aktivieren
      }
      else if (menuIndex == 1) {
        setCurrentMenu(CALIBRATE_HUMIDITY_INSIDE);
        editMode = true; // Editiermodus aktivieren
      }
      else if (menuIndex == 2) setCurrentMenu(CALIBRATE_SENSOR); // Zurück zum Kalibriermenü
      break;

    case CALIBRATE_OUTSIDE:
      if (menuIndex == 0) {
        setCurrentMenu(CALIBRATE_TEMP_OUTSIDE);
        editMode = true; // Editiermodus aktivieren
      }
      else if (menuIndex == 1) {
        setCurrentMenu(CALIBRATE_HUMIDITY_OUTSIDE);
        editMode = true; // Editiermodus aktivieren
      }
      else if (menuIndex == 2) setCurrentMenu(CALIBRATE_SENSOR); // Zurück zum Kalibriermenü
      break;
  }
}

// Funktion zur Setzen des aktuellen Menüs und Reset des Index
void setCurrentMenu(Menu newMenu) {
  currentMenu = newMenu;
  menuIndex = 0;  // Reset des Index für neues Untermenü
}

// Funktion zur Anzeige während des Bearbeitens und der Darstellung eines Wertes
void displayEditOrShow(float value, const char* unit) {
  lcd.setCursor(0, 1);
  lcd.print(value, 1);
  lcd.print(unit);

  if (editMode) {
    lcd.setCursor(15, 1);
    lcd.print("*"); // Kennzeichnet den Bearbeitungsmodus
  }
}

// Funktion zur Aktualisierung des Rotary Encoders
void updateEncoder() {
  int MSB = digitalRead(ENCODER_PIN_A);
  int LSB = digitalRead(ENCODER_PIN_B);

  // Kodierung des Encodersignals
  int encoded = (MSB << 1) | LSB;
  int sum = (lastEncoded << 2) | encoded;

  // Erhöhung/Erniedrigung des Schrittzählers
  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) {
    stepCounter++;
  }
  if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) {
    stepCounter--;
  }

  // Inkrementelle Anpassung des Menüindex oder Wertes
  if (stepCounter >= 4) {
    stepCounter = 0;
    if (editMode) {
      handleValueChange(1); // Nur im Bearbeitungsmodus ändern
    } else if (inMenu) {
      menuIndex += 1;
    }
  } else if (stepCounter <= -4) {
    stepCounter = 0;
    if (editMode) {
      handleValueChange(-1); // Nur im Bearbeitungsmodus ändern
    } else if (inMenu) {
      menuIndex -= 1;
    }
  }

  lastEncoded = encoded;
}

// Begrenzung des Menüindexje nach Menübereich
void limitMenuIndex() {
  int maxIndex = 0;
  switch (currentMenu) {
    case MAIN: maxIndex = 6; break;
    case MODE_SELECTION: maxIndex = 2; break;
    case FAN_MANUAL: maxIndex = 3; break;
    case CALIBRATE_SENSOR: maxIndex = 2; break;
    case TARGET_TEMP:
    case TARGET_HUM:
    case MIN_TEMP:
      maxIndex = 0; break; // Kein Index im Wertemenü
    case CALIBRATE_INSIDE:
    case CALIBRATE_OUTSIDE:
      maxIndex = 2; break;
    case CALIBRATE_TEMP_INSIDE:
    case CALIBRATE_HUMIDITY_INSIDE:
    case CALIBRATE_TEMP_OUTSIDE:
    case CALIBRATE_HUMIDITY_OUTSIDE:
      maxIndex = 0; break; // Kein Index im Wertemenü
  }

  if (menuIndex < 0) menuIndex = maxIndex;
  if (menuIndex > maxIndex) menuIndex = 0;
}

// Funktion zur Anpassung eines Werts (nur im Bearbeitungsmodus)
void handleValueChange(int direction) {
  float delta = 0.1 * direction; // Schrittgröße

  switch (currentMenu) {
    case TARGET_TEMP:
      targetTemp += delta;
      break;
    case TARGET_HUM:
      targetHum += delta;
      if (targetHum > 100.0) targetHum = 100.0;
      if (targetHum < 0.0) targetHum = 0.0;
      break;
    case MIN_TEMP:
      minTemp += delta;
      break;
    case CALIBRATE_TEMP_INSIDE:
      tempOffsetInsideTemp += delta;
      break;
    case CALIBRATE_HUMIDITY_INSIDE:
      tempOffsetInsideHum += delta;
      break;
    case CALIBRATE_TEMP_OUTSIDE:
      tempOffsetOutsideTemp += delta;
      break;
    case CALIBRATE_HUMIDITY_OUTSIDE:
      tempOffsetOutsideHum += delta;
      break;
  }
}

// Funktion zum Laden der Einstellungen aus dem EEPROM
void loadSettings() {
  // Prüfung, ob EEPROM initialisiert ist
  if (EEPROM.read(EEPROM_ADDRESS) == 123) {
    // Werte aus EEPROM laden
    EEPROM.get(1, targetTemp);
    EEPROM.get(5, targetHum);
    EEPROM.get(9, minTemp);
    EEPROM.get(13, tempOffsetInsideTemp);
    EEPROM.get(17, tempOffsetInsideHum);
    EEPROM.get(21, tempOffsetOutsideTemp);
    EEPROM.get(25, tempOffsetOutsideHum);

    // Lade auch den gespeicherten Modus, wenn vorhanden
    byte savedMode = EEPROM.read(29);
    if (savedMode == 0 || savedMode == 1) {
      mode = (Mode)savedMode;
      previousMode = mode; // Setze auch previousMode, damit kein sofortiger Offset angewendet wird
    }
  }

  // Fallback-Werte, falls keine gültigen Werte im EEPROM sind
  if (isnan(targetTemp)) targetTemp = 15.0;
  if (isnan(targetHum)) targetHum = 55.0;
  if (isnan(minTemp)) minTemp = 5.0;
  if (isnan(tempOffsetInsideTemp)) tempOffsetInsideTemp = 0.0;
  if (isnan(tempOffsetInsideHum)) tempOffsetInsideHum = 0.0;
  if (isnan(tempOffsetOutsideTemp)) tempOffsetOutsideTemp = 0.0;
  if (isnan(tempOffsetOutsideHum)) tempOffsetOutsideHum = 0.0;
}

// Funktion zum Speichern der aktuellen Einstellungen im EEPROM
void saveSettings() {
  EEPROM.write(EEPROM_ADDRESS, 123); // Markierung für Initialisierung setzen
  EEPROM.put(1, targetTemp);
  EEPROM.put(5, targetHum);
  EEPROM.put(9, minTemp);
  EEPROM.put(13, tempOffsetInsideTemp);
  EEPROM.put(17, tempOffsetInsideHum);
  EEPROM.put(21, tempOffsetOutsideTemp);
  EEPROM.put(25, tempOffsetOutsideHum);

  // Speichere auch den aktuellen Modus
  EEPROM.write(29, (byte)mode);
}
