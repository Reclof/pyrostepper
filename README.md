# pyrostepper
Opensource Feuerwerkzündanlage (Stepper)

Arduino Projekt.

Diese Projekt beschäftigt sich mit einer selbstgebauten Feuerwerkzündanlage:

## Funktionsbeschreibung:

- Arduino Uno 
- 24 Kanäle nacheinander zeitgesteuert zünden.
- Speicherung der unterschiedlichen Laufzeiten je Kanal bis zur nächsten Zündung
- Sekundengenaue Taktung (Mehr ist mit meinem Hobby nicht erfroderlich), eine Zündung mit 1/10 oder genauer Sekundenbereich würde sich bei Anpassung umsetzen lassen.
- LCD Anzeige auf 20x4 zur Programmierung
- 7-Segmentanzeige zur schnellen Info aus größerer Entfernung
- 24 LEDs zeigen den Kanal an, wenn dieser zündet
- Galvanische Trennung zwischen Elektronik und Zündkreis. 
- Zündkreispannung ab 6V oder höher
- USB Power Pack zur Stromversorgung der Elektronik und des Arduinos
