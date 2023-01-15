# pyrostepper
Opensource Feuerwerkzündanlage (Stepper)

### Ein Arduino Projekt von Reclof.

Diese Projekt beschäftigt sich mit einer selbstgebauten Feuerwerkzündanlage:
Warum? Weil ich lust darauf habe und ich es kann :). Es gib sicherlich auch fertige Anlagen auf dem Markt, aber selber bauen macht mehr spaß!

## Funktionsbeschreibung:

- Arduino Uno 
- 24 Kanäle nacheinander zeitgesteuert zünden.
- Speicherung der unterschiedlichen Laufzeiten je Kanal bis zur nächsten Zündung
- Sekundengenaue Taktung (genauer ist mit meinem Hobby nicht erforderlich), eine Zündung mit 1/10 oder genauer würde sich bei Anpassung umsetzen lassen.
- LCD Anzeige auf 20x4 zur Programmierung
- 7-Segmentanzeige zur schnellen Info aus größerer Entfernung
- 24 LEDs zeigen den Kanal an, wenn dieser zündet
- Galvanische Trennung zwischen Elektronik und Zündkreis. 
- Zündkreispannung ab 6V oder höher
- USB Power Pack zur Stromversorgung der Elektronik und des Arduinos
