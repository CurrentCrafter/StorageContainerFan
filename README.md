# StorageContainerFan

Kleines Programm für einen Arduino Nano mit folgender Peripherie:

- 2x DHT22 für innen und außen
- Relais für Lüfteransteuerung
- Rotary Encoder
- Toggle Switch für Sommer und Wintermodus

## Programm soll:

- Immer:
  - Innentemperatur möglichst gering, jedoch immer über 5°C
  - (Keine Kondensation im Innenraum)
  - Luftfeuchtigkeit unter einstellbarem Schwellenwert halten (im Zweifel lüfter an, wenn OutsideHum<InsideHum) jedoch:
- Im Sommermodus: Primär die tiefstmögliche Temperatur anstreben, ohne die Luftfeuchtigkeit drastisch zu steigern. Bei Hum. überschreitung soll der Lüfter nur bei Außentemp: 2 °C über der Innentemperatur angehen.
- Im Wintermodus: Wenn draußen wärmer als drinnen, jedoch <15°C -> lüften.
- Veränderte Schwellenwerte in der EEProm Speichern und eine Möglichkeit haben die Werte in der EEPROM zurückzusetzen
- Die EEPROM Darf nur sprasam benutzt werden (langleibigkeit)

## Pinbelegung:
```C
DHTPIN_INSIDE 5    // Pin für den Innensensor DHT22
DHTPIN_OUTSIDE 6   // Pin für den Außensensor DHT22
DHTTYPE DHT22      // DHT22 (AM2302) Sensortyp
RELAY_PIN 7        // Pin für das Relais zur Lüftersteuerung
ENCODER_PIN_A 2    // Rotary Encoder Pin A (CLK)
ENCODER_PIN_B 3    // Rotary Encoder Pin B (DT)
ENCODER_SW 4       // Rotary Encoder Switch
TOGGLE_WINTER 8    //Toggle for Wintermode (D8)
TOGGLE_SUMMER 9    //Toggle for Summermode (D9)
```
- Wenn 9 = Low -> Sommer (interner Pullup)
- Wenn 8 = Low -> Winter (interner Pullup)
- Wenn 8 & 9 = HIGH -> Nur Temp / Hum anzeigen. 
- Jeden der Drei Zustände auch im Display verdeutlichen

> Beachte: Die Relaislogik ist invertiert.

## Display:
- 16 Zeichen, 2 Zeilen
- 1. Zeile: Ident für Winter (W), Sommer (S), Nur Messen (M),Leerzeichen,Leerzeichen,I:XX.X,Leerzeichen,A:XX.X 
- 2. Zeile: LF,Leerzeichen,I:XX.X,Leerzeichen,A:XX.X
   
