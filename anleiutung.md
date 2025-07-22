# Bedienungsanleitung: Container-Lüftersteuerung V3

## Übersicht
Diese intelligente Lüftersteuerung überwacht automatisch Temperatur und Luftfeuchtigkeit in Ihrem Container und steuert den Lüfter entsprechend, um optimale Lagerbedingungen zu gewährleisten.

---

## 1. Display verstehen

### Hauptanzeige
Das 16x2 LCD-Display zeigt kontinuierlich wichtige Informationen an:

**Zeile 1:** `S  I:22.5 A:18.3`
- **S/W/M**: Betriebsmodus (Sommer/Winter/Nur Messen)
- **I:**: Innentemperatur in °C
- **A:**: Außentemperatur in °C

**Zeile 2:** `LF I:65.2 A:58.7 ON`
- **LF**: Luftfeuchtigkeit (Luftfeuchtigkeit)
- **I:**: Luftfeuchtigkeit innen in %
- **A:**: Luftfeuchtigkeit außen in %
- **ON**: Erscheint wenn der Lüfter läuft

### Fehlermeldungen
- **XX.X**: Sensor defekt oder nicht angeschlossen
- **--.-**: Temporärer Messfehler

---

## 2. Betriebsmodi

### Modus-Schalter (3-Positionen)
Der Hauptschalter hat drei Stellungen:

#### **Sommermodus (S)**
- **Ziel**: Temperatur so niedrig wie möglich halten
- **Lüfter läuft wenn**:
  - Außentemperatur niedriger als innen UND
  - Luftfeuchtigkeit unter dem Grenzwert ODER
  - Außentemperatur max. 2°C unter Innentemperatur bei Feuchtigkeitsüberschreitung
- **Ideal für**: Warme Jahreszeit, Schutz vor Überhitzung

#### **Wintermodus (W)**
- **Ziel**: Aufwärmung bei günstigen Bedingungen
- **Lüfter läuft wenn**:
  - Außentemperatur wärmer als innen aber unter 15°C ODER
  - Luftfeuchtigkeit zu hoch und außen trockener
- **Ideal für**: Kalte Jahreszeit, Kondensationsvermeidung

#### **Nur Messen (M)**
- **Lüfter ist ausgeschaltet**
- **Nur Temperatur- und Feuchtigkeitsanzeige**
- **Ideal für**: Überwachung ohne Lüftung, Service

---

## 3. Sicherheitsfunktionen

### Automatische Schutzfunktionen
- **Mindesttemperatur**: Lüfter läuft nie wenn Innentemperatur unter 5°C fallen würde
- **Sensorüberwachung**: Bei Sensorausfall wird Lüfter gestoppt
- **Minimale Laufzeit**: Lüfter läuft mindestens 1 Minute um häufiges Ein/Ausschalten zu vermeiden

---

## 4. Menü-System bedienen

### Menü öffnen
1. **Encoder-Taste drücken** um ins Hauptmenü zu gelangen

### Hauptmenü-Optionen
```
MENU:
1. Ziel Temp       - Gewünschte Zieltemperatur einstellen
2. Ziel Feuchte    - Maximale Luftfeuchtigkeit einstellen
3. Min Temp        - Mindesttemperatur festlegen
4. Kalibrierung    - Sensoren kalibrieren (Experteneinstellung)
5. Reset EEPROM    - Alle Einstellungen zurücksetzen
6. Zurück          - Zurück zur Hauptanzeige
```

### Navigation
- **Encoder drehen**: Zwischen Menüpunkten wechseln
- **Encoder drücken**: Menüpunkt auswählen
- **In Untermenüs**: Encoder drehen = Zurück zum Hauptmenü

### Werte ändern
1. **Menüpunkt auswählen** (z.B. "1. Ziel Temp")
2. **Encoder drücken** → Editiermodus (zeigt `<` Symbol)
3. **Encoder drehen** → Wert ändern
4. **Encoder drücken** → Wert speichern und Editiermodus verlassen

---

## 5. Wichtige Einstellungen

### **Zieltemperatur** (Standard: 25.0°C)
- **Bereich**: 5.0°C - 40.0°C
- **Schritte**: 0.5°C
- **Zweck**: Gewünschte Innentemperatur

### **Ziel Luftfeuchtigkeit** (Standard: 45.0%)
- **Bereich**: 30.0% - 90.0%
- **Schritte**: 5.0%
- **Zweck**: Maximale erlaubte Luftfeuchtigkeit

### **Mindesttemperatur** (Standard: 5.0°C)
- **Bereich**: 0.0°C - 15.0°C
- **Schritte**: 0.5°C
- **Zweck**: Frostschutz - Lüfter läuft nie unter diesem Wert

---

## 6. Erste Inbetriebnahme

### Checkliste
1. Stromversorgung anschließen
2. Beide DHT22-Sensoren angeschlossen (innen/außen)
3. Lüfter-Relais angeschlossen
4. Kurzer Relais-Test beim Start (Lüfter geht kurz an)
5. LCD zeigt Sensorwerte korrekt an

### Grundkonfiguration
1. **Modus wählen**: Schalter auf gewünschte Position
2. **Zielwerte einstellen**: Menü → Zieltemperatur und Luftfeuchtigkeit
3. **Mindesttemperatur prüfen**: Je nach Lagerinhalt anpassen
4. **Automatik testen**: In verschiedenen Modi beobachten

---

## 7. Wartung und Pflege

### Regelmäßige Kontrollen
- **Wöchentlich**: Sensorwerte auf Plausibilität prüfen
- **Monatlich**: Lüfter auf Verschmutzung kontrollieren
- **Saisonal**: Einstellungen an Jahreszeit anpassen

### Reinigung
- **Sensoren**: Vorsichtig mit trockenem Pinsel entstauben
- **Display**: Mit feuchtem Tuch abwischen
- **Lüfter**: Gitter reinigen, Laufgeräusche beachten

### Einstellungen sichern
- **Automatisch**: Alle Änderungen werden automatisch gespeichert
- **Reset**: Bei Problemen "Reset EEPROM" verwenden
- **Backup**: Wichtige Werte notieren

---

## 8. Problembehandlung

### Häufige Probleme

#### **Display zeigt XX.X oder --.-**
- **Ursache**: Sensor defekt oder nicht angeschlossen
- **Lösung**: Verkabelung prüfen, Sensor austauschen

#### **Lüfter läuft nicht**
- **Prüfen**: Ist Modus auf "M" (Nur Messen)?
- **Prüfen**: Relais-Verkabelung korrekt?


#### **Lüfter läuft ständig**
- **Prüfen**: Zielwerte realistisch?
- **Prüfen**: Sensoren korrekt positioniert?
- **Prüfen**: Luftzirkulation im Container?

#### **Modus wechselt nicht**
- **Prüfen**: Schalter-Verkabelung (mittlerer Pol an GND)
- **Prüfen**: Kontakte sauber?

#### **Einstellungen gehen verloren**
- **Ursache**: EEPROM-Problem
- **Lösung**: "Reset EEPROM" ausführen, neu konfigurieren

---

## 9. Technische Daten

### Spezifikationen
- **Stromversorgung**: 5V DC (Arduino Nano)
- **Sensoren**: 2x DHT22 (Temperatur: ±0.5°C, Luftfeuchte: ±2%)
- **Display**: 16x2 LCD mit I2C
- **Relais**: 1x für Lüftersteuerung (invertierte Logik)
- **Speicher**: EEPROM für dauerhafte Einstellungen

### Anschlüsse
- **DHT22 Innen**: Pin 5
- **DHT22 Außen**: Pin 6
- **Relais**: Pin 7
- **Encoder A**: Pin 2
- **Encoder B**: Pin 3
- **Encoder Taste**: Pin 4
- **Winterschalter**: Pin 8
- **Sommerschalter**: Pin 9

---

## 10. Tipps für optimale Nutzung

### **Sommerbetrieb**
- Zieltemperatur niedriger setzen (20-25°C)
- Luftfeuchtigkeit streng begrenzen (50-60%)
- Regelmäßig Kondensation kontrollieren

### **Winterbetrieb**
- Mindesttemperatur erhöhen (8-10°C)
- Luftfeuchtigkeit lockerer handhaben (60-70%)
- Auf Frostbildung achten

### **Lagerinhalt berücksichtigen**
- **Elektronik**: Niedrige Feuchtigkeit wichtig
- **Textilien**: Mittlere Werte ausreichend
- **Metallgegenstände**: Rostschutz durch niedrige Feuchtigkeit

---

## Support

Bei technischen Problemen:
1. **Checkliste** in Abschnitt 8 durchgehen
2. **Einstellungen zurücksetzen** (Reset EEPROM)
3. **Verkabelung überprüfen**
4. **Bei anhaltenden Problemen**: Fachhändler kontaktieren

---

**Version**: 3.0  
**Stand**: Juli 2025  
**Kompatibel mit**: Arduino Nano, DHT22-Sensoren, I2C-LCD
