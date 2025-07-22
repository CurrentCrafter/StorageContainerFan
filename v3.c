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
#define TOGGLE_WINTER 8    // Toggle for Wintermode (D8)
#define TOGGLE_SUMMER 9    // Toggle for Summermode (D9)

// Definition für die invertierte Logik des Lüfterrelais
#define FAN_ON 0           // Lüfter an (LOW bei invertierter Logik)
#define FAN_OFF 1          // Lüfter aus (HIGH bei invertierter Logik)

// Konstanten für Temperaturschwellenwerte
#define MIN_TEMP_THRESHOLD 5.0      // Minimum Temperatur (°C)
#define MAX_WINTER_TEMP 15.0        // Maximum Temperatur für Winterlüftung (°C)
#define SUMMER_TEMP_OFFSET 2.0      // Temperaturoffset für Sommerlüftung (°C)

// EEPROM Adressen für gespeicherte Werte
#define EEPROM_INIT_FLAG 0
#define EEPROM_TARGET_TEMP 4
#define EEPROM_TARGET_HUM 8
#define EEPROM_MIN_TEMP 12
#define EEPROM_TEMP_OFFSET_IN 16
#define EEPROM_HUM_OFFSET_IN 20
#define EEPROM_TEMP_OFFSET_OUT 24
#define EEPROM_HUM_OFFSET_OUT 28
#define EEPROM_INIT_VALUE 0xAA55  // Initialisierungswert für EEPROM

// Initialisierung der Komponenten
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht_inside(DHTPIN_INSIDE, DHTTYPE);
DHT dht_outside(DHTPIN_OUTSIDE, DHTTYPE);

// Globale Variablen für den Rotary Encoder
volatile int encoderPos = 0;
volatile bool encoderChanged = false;
int lastEncoderA = HIGH;

// Betriebsmodi basierend auf Toggle-Switches
enum OperationMode {
  SUMMER_MODE,    // Pin 9 LOW, Pin 8 HIGH
  WINTER_MODE,    // Pin 8 LOW, Pin 9 HIGH  
  MEASURE_ONLY    // Pin 8 HIGH, Pin 9 HIGH
};
OperationMode currentMode = MEASURE_ONLY;

// Lüfterstatus
bool fanActive = false;
unsigned long fanOnTimer = 0;
const unsigned long FAN_MIN_ON_TIME = 60000;  // Minimum Laufzeit 1 Minute

// Menüsystem
enum MenuState {
  MAIN_DISPLAY,
  MENU_ROOT,
  MENU_SET_TARGET_TEMP,
  MENU_SET_TARGET_HUM,
  MENU_SET_MIN_TEMP,
  MENU_CALIBRATE,
  MENU_RESET_EEPROM
};
MenuState menuState = MAIN_DISPLAY;
int menuIndex = 0;
bool inEditMode = false;

// Konfigurierbare Parameter
float targetTemp = 25.0;
float targetHumidity = 60.0;
float minTemp = 5.0;
float tempOffsetInside = 0.0;
float humOffsetInside = 0.0;
float tempOffsetOutside = 0.0;
float humOffsetOutside = 0.0;

// Timing-Variablen
unsigned long lastSensorRead = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastEEPROMSave = 0;
unsigned long buttonPressTime = 0;
const unsigned long SENSOR_INTERVAL = 2000;      // Sensor alle 2 Sekunden lesen
const unsigned long DISPLAY_INTERVAL = 500;      // Display alle 500ms aktualisieren
const unsigned long EEPROM_SAVE_INTERVAL = 10000; // EEPROM alle 10 Sekunden speichern
const unsigned long BUTTON_DEBOUNCE = 200;       // Button Debounce Zeit

// Sensordaten
float insideTemp, outsideTemp, insideHum, outsideHum;
bool sensorsValid = false;

// Funktion Prototypen
void readSensors();
void updateDisplay();
void controlFan();
void handleEncoder();
void handleButton();
void updateMenu();
void saveToEEPROM();
void loadFromEEPROM();
void resetEEPROM();
OperationMode readMode();
void encoderInterrupt();

void setup() {
  // Pin-Konfiguration
  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);
  pinMode(ENCODER_SW, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(TOGGLE_WINTER, INPUT_PULLUP);
  pinMode(TOGGLE_SUMMER, INPUT_PULLUP);
  
  // Lüfter initial ausschalten
  digitalWrite(RELAY_PIN, FAN_OFF);
  
  // Encoder Interrupt konfigurieren
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), encoderInterrupt, CHANGE);
  
  // LCD initialisieren
  lcd.begin(16, 2);
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Container Fan V3");
  lcd.setCursor(0, 1);
  lcd.print("Initialisierung");
  
  // DHT Sensoren initialisieren
  dht_inside.begin();
  dht_outside.begin();
  
  // EEPROM laden
  loadFromEEPROM();
  
  // Kurzer Relais-Test
  digitalWrite(RELAY_PIN, FAN_ON);
  delay(200);
  digitalWrite(RELAY_PIN, FAN_OFF);
  
  delay(2000);
  lcd.clear();
}

void loop() {
  unsigned long currentTime = millis();
  
  // Betriebsmodus lesen
  currentMode = readMode();
  
  // Sensoren lesen
  if (currentTime - lastSensorRead >= SENSOR_INTERVAL) {
    readSensors();
    lastSensorRead = currentTime;
  }
  
  // Lüftersteuerung (nur wenn nicht im MEASURE_ONLY Modus)
  if (currentMode != MEASURE_ONLY) {
    controlFan();
  } else {
    // Im Messmodus Lüfter ausschalten
    digitalWrite(RELAY_PIN, FAN_OFF);
    fanActive = false;
  }
  
  // Encoder und Button verarbeiten
  handleEncoder();
  handleButton();
  
  // Display aktualisieren
  if (currentTime - lastDisplayUpdate >= DISPLAY_INTERVAL) {
    updateDisplay();
    lastDisplayUpdate = currentTime;
  }
  
  // EEPROM sparsam speichern
  if (currentTime - lastEEPROMSave >= EEPROM_SAVE_INTERVAL) {
    saveToEEPROM();
    lastEEPROMSave = currentTime;
  }
}

void readSensors() {
  // Temperatur und Feuchtigkeit lesen mit Kalibrierungsoffsets
  insideTemp = dht_inside.readTemperature() + tempOffsetInside;
  outsideTemp = dht_outside.readTemperature() + tempOffsetOutside;
  insideHum = dht_inside.readHumidity() + humOffsetInside;
  outsideHum = dht_outside.readHumidity() + humOffsetOutside;
  
  // Validierung der Sensordaten
  sensorsValid = !isnan(insideTemp) && !isnan(outsideTemp) && 
                 !isnan(insideHum) && !isnan(outsideHum);
}

void controlFan() {
  if (!sensorsValid) {
    // Bei Sensorfehlern Lüfter ausschalten
    digitalWrite(RELAY_PIN, FAN_OFF);
    fanActive = false;
    return;
  }
  
  bool shouldRunFan = false;
  
  // Sicherheitscheck: Nie unter Mindesttemperatur lüften
  if (insideTemp <= minTemp && outsideTemp < insideTemp) {
    shouldRunFan = false;
  }
  else {
    switch (currentMode) {
      case SUMMER_MODE:
        // Sommermodus: Primär Temperatur senken, Feuchtigkeit beachten
        if (outsideTemp < insideTemp) {
          if (insideHum <= targetHumidity) {
            // Feuchtigkeit OK, lüften wenn Außentemperatur niedriger
            shouldRunFan = true;
          } else if (outsideTemp >= (insideTemp - SUMMER_TEMP_OFFSET)) {
            // Feuchtigkeit zu hoch, nur lüften wenn Außentemperatur nicht viel niedriger
            shouldRunFan = (outsideHum < insideHum);
          }
        }
        // Zusätzlich: Lüften wenn Außenfeuchtigkeit deutlich niedriger
        if (insideHum > targetHumidity && outsideHum < (insideHum - 10)) {
          shouldRunFan = true;
        }
        break;
        
      case WINTER_MODE:
        // Wintermodus: Lüften wenn draußen wärmer als drinnen, aber unter 15°C
        if (outsideTemp > insideTemp && outsideTemp < MAX_WINTER_TEMP) {
          shouldRunFan = true;
        }
        // Oder bei zu hoher Feuchtigkeit und niedrigerer Außenfeuchtigkeit
        if (insideHum > targetHumidity && outsideHum < insideHum && outsideTemp > minTemp) {
          shouldRunFan = true;
        }
        break;
        
      case MEASURE_ONLY:
        shouldRunFan = false;
        break;
    }
  }
  
  // Lüftersteuerung mit Hysterese
  if (shouldRunFan && !fanActive) {
    digitalWrite(RELAY_PIN, FAN_ON);
    fanActive = true;
    fanOnTimer = millis();
  } else if (!shouldRunFan && fanActive) {
    // Lüfter mindestens 1 Minute laufen lassen
    if (millis() - fanOnTimer >= FAN_MIN_ON_TIME) {
      digitalWrite(RELAY_PIN, FAN_OFF);
      fanActive = false;
    }
  }
}

void updateDisplay() {
  if (menuState == MAIN_DISPLAY) {
    lcd.clear();
    
    // Erste Zeile: Modus-Ident, Temperaturen
    lcd.setCursor(0, 0);
    switch (currentMode) {
      case SUMMER_MODE: lcd.print("S "); break;
      case WINTER_MODE: lcd.print("W "); break;
      case MEASURE_ONLY: lcd.print("M "); break;
    }
    
    lcd.print("I:");
    if (sensorsValid) {
      lcd.print(insideTemp, 1);
    } else {
      lcd.print("--.-");
    }
    
    lcd.print(" A:");
    if (sensorsValid) {
      lcd.print(outsideTemp, 1);
    } else {
      lcd.print("--.-");
    }
    
    // Zweite Zeile: Luftfeuchtigkeit und Lüfterstatus
    lcd.setCursor(0, 1);
    lcd.print("LF I:");
    if (sensorsValid) {
      lcd.print(insideHum, 0);
    } else {
      lcd.print("--");
    }
    
    lcd.print(" A:");
    if (sensorsValid) {
      lcd.print(outsideHum, 0);
    } else {
      lcd.print("--");
    }
    
    // Lüfterstatus anzeigen
    if (fanActive) {
      lcd.print(" ON");
    }
  } else {
    updateMenu();
  }
}

void updateMenu() {
  lcd.clear();
  
  switch (menuState) {
    case MENU_ROOT:
      lcd.setCursor(0, 0);
      lcd.print("MENU:");
      lcd.setCursor(0, 1);
      switch (menuIndex) {
        case 0: lcd.print("1.Ziel Temp"); break;
        case 1: lcd.print("2.Ziel Feuchte"); break;
        case 2: lcd.print("3.Min Temp"); break;
        case 3: lcd.print("4.Kalibrierung"); break;
        case 4: lcd.print("5.Reset EEPROM"); break;
        case 5: lcd.print("6.Zurueck"); break;
      }
      break;
      
    case MENU_SET_TARGET_TEMP:
      lcd.setCursor(0, 0);
      lcd.print("Ziel Temp:");
      lcd.setCursor(0, 1);
      lcd.print(targetTemp, 1);
      lcd.print(" C");
      if (inEditMode) lcd.print(" <");
      break;
      
    case MENU_SET_TARGET_HUM:
      lcd.setCursor(0, 0);
      lcd.print("Ziel Feuchte:");
      lcd.setCursor(0, 1);
      lcd.print(targetHumidity, 0);
      lcd.print(" %");
      if (inEditMode) lcd.print(" <");
      break;
      
    case MENU_SET_MIN_TEMP:
      lcd.setCursor(0, 0);
      lcd.print("Min Temp:");
      lcd.setCursor(0, 1);
      lcd.print(minTemp, 1);
      lcd.print(" C");
      if (inEditMode) lcd.print(" <");
      break;
      
    case MENU_CALIBRATE:
      lcd.setCursor(0, 0);
      lcd.print("Kalibrierung");
      lcd.setCursor(0, 1);
      lcd.print("Nicht impl.");
      break;
      
    case MENU_RESET_EEPROM:
      lcd.setCursor(0, 0);
      lcd.print("EEPROM Reset");
      lcd.setCursor(0, 1);
      lcd.print("Druecken = Reset");
      break;
  }
}

void handleEncoder() {
  if (encoderChanged) {
    encoderChanged = false;
    
    if (menuState == MAIN_DISPLAY) {
      // Im Hauptdisplay macht der Encoder nichts
      return;
    }
    
    if (inEditMode) {
      // Werte ändern
      switch (menuState) {
        case MENU_SET_TARGET_TEMP:
          targetTemp += encoderPos * 0.5;
          targetTemp = constrain(targetTemp, 5.0, 40.0);
          break;
        case MENU_SET_TARGET_HUM:
          targetHumidity += encoderPos * 5.0;
          targetHumidity = constrain(targetHumidity, 30.0, 90.0);
          break;
        case MENU_SET_MIN_TEMP:
          minTemp += encoderPos * 0.5;
          minTemp = constrain(minTemp, 0.0, 15.0);
          break;
      }
    } else {
      // Menü navigieren
      if (menuState == MENU_ROOT) {
        menuIndex += encoderPos;
        menuIndex = constrain(menuIndex, 0, 5);
      }
    }
    
    encoderPos = 0;
  }
}

void handleButton() {
  static bool lastButtonState = HIGH;
  bool currentButtonState = digitalRead(ENCODER_SW);
  
  if (currentButtonState == LOW && lastButtonState == HIGH) {
    if (millis() - buttonPressTime > BUTTON_DEBOUNCE) {
      buttonPressTime = millis();
      
      if (menuState == MAIN_DISPLAY) {
        menuState = MENU_ROOT;
        menuIndex = 0;
      } else if (inEditMode) {
        inEditMode = false;
        saveToEEPROM();
      } else {
        switch (menuState) {
          case MENU_ROOT:
            switch (menuIndex) {
              case 0: menuState = MENU_SET_TARGET_TEMP; break;
              case 1: menuState = MENU_SET_TARGET_HUM; break;
              case 2: menuState = MENU_SET_MIN_TEMP; break;
              case 3: menuState = MENU_CALIBRATE; break;
              case 4: 
                menuState = MENU_RESET_EEPROM;
                break;
              case 5: 
                menuState = MAIN_DISPLAY;
                break;
            }
            break;
            
          case MENU_SET_TARGET_TEMP:
          case MENU_SET_TARGET_HUM:
          case MENU_SET_MIN_TEMP:
            if (!inEditMode) {
              inEditMode = true;
            }
            break;
            
          case MENU_RESET_EEPROM:
            resetEEPROM();
            menuState = MAIN_DISPLAY;
            break;
            
          case MENU_CALIBRATE:
            menuState = MENU_ROOT;
            break;
        }
      }
    }
  }
  
  lastButtonState = currentButtonState;
}

OperationMode readMode() {
  bool winter = (digitalRead(TOGGLE_WINTER) == LOW);
  bool summer = (digitalRead(TOGGLE_SUMMER) == LOW);
  
  if (summer && !winter) {
    return SUMMER_MODE;
  } else if (winter && !summer) {
    return WINTER_MODE;
  } else {
    return MEASURE_ONLY;
  }
}

void encoderInterrupt() {
  int currentA = digitalRead(ENCODER_PIN_A);
  int currentB = digitalRead(ENCODER_PIN_B);
  
  if (currentA != lastEncoderA) {
    if (currentA == currentB) {
      encoderPos--;
    } else {
      encoderPos++;
    }
    encoderChanged = true;
  }
  
  lastEncoderA = currentA;
}

void saveToEEPROM() {
  // Nur speichern wenn Werte geändert wurden
  static float lastTargetTemp = 0;
  static float lastTargetHum = 0;
  static float lastMinTemp = 0;
  
  bool needsSave = (lastTargetTemp != targetTemp) || 
                   (lastTargetHum != targetHumidity) || 
                   (lastMinTemp != minTemp);
  
  if (needsSave) {
    EEPROM.put(EEPROM_TARGET_TEMP, targetTemp);
    EEPROM.put(EEPROM_TARGET_HUM, targetHumidity);
    EEPROM.put(EEPROM_MIN_TEMP, minTemp);
    EEPROM.put(EEPROM_TEMP_OFFSET_IN, tempOffsetInside);
    EEPROM.put(EEPROM_HUM_OFFSET_IN, humOffsetInside);
    EEPROM.put(EEPROM_TEMP_OFFSET_OUT, tempOffsetOutside);
    EEPROM.put(EEPROM_HUM_OFFSET_OUT, humOffsetOutside);
    
    uint16_t initFlag = EEPROM_INIT_VALUE;
    EEPROM.put(EEPROM_INIT_FLAG, initFlag);
    
    lastTargetTemp = targetTemp;
    lastTargetHum = targetHumidity;
    lastMinTemp = minTemp;
  }
}

void loadFromEEPROM() {
  uint16_t initFlag;
  EEPROM.get(EEPROM_INIT_FLAG, initFlag);
  
  if (initFlag == EEPROM_INIT_VALUE) {
    EEPROM.get(EEPROM_TARGET_TEMP, targetTemp);
    EEPROM.get(EEPROM_TARGET_HUM, targetHumidity);
    EEPROM.get(EEPROM_MIN_TEMP, minTemp);
    EEPROM.get(EEPROM_TEMP_OFFSET_IN, tempOffsetInside);
    EEPROM.get(EEPROM_HUM_OFFSET_IN, humOffsetInside);
    EEPROM.get(EEPROM_TEMP_OFFSET_OUT, tempOffsetOutside);
    EEPROM.get(EEPROM_HUM_OFFSET_OUT, humOffsetOutside);
  } else {
    // Standardwerte verwenden
    resetEEPROM();
  }
}

void resetEEPROM() {
  // Standardwerte setzen
  targetTemp = 25.0;
  targetHumidity = 60.0;
  minTemp = 5.0;
  tempOffsetInside = 0.0;
  humOffsetInside = 0.0;
  tempOffsetOutside = 0.0;
  humOffsetOutside = 0.0;
  
  // Sofort speichern
  lastEEPROMSave = 0; // Force save
  saveToEEPROM();
  
  // Bestätigung anzeigen
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("EEPROM Reset");
  lcd.setCursor(0, 1);
  lcd.print("Abgeschlossen!");
  delay(2000);
}
