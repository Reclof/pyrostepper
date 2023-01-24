#include <Wire.h>  // lcd(i2c)
#include <jm_LCM2004A_I2C.h> // lcd 20x4
#include <EEPROM.h> // für die Speicherung der Programmwerte (Ich nutze zZ nur ein Byte, das reicht für jeweils 255 Sekunden)

//**************************************************************//
//  Name    : Feuerwerk Sequenzer                               //
//  Author  : Folker Axmann                                     //
//  Date    : 14.11.2020      15.01.2023      22.01.2023        //
//  Version : 1.0             1.1             1.2               //
//  Notes   : nutzt 4 74HC595 Shift Register                    //
//          : eine 7 Segmentanzeige                             //
//          : 3 mal 8 Relais                                    //
//          : 20x4 LCD Display                                  //
//          : Taster und Schalter                               //
//          : 2x Mux       74HC4067                             // 
//**************************************************************//
// es wird pro sequenzt ein Relaus angezogen (für 600Milisekunden), wenn mehrere Kanäle 
// gleichzeitg geschaktet werden sollen, so muss das skript angepasst werden
// für den Stand 2020 reicht das voll aus, ich benötige nur ein einzigen Kanal je , Zündbrücken können parallel oder in Reihe geschlatet werden
//
// WICHTIG!
//
// AUFGRUND von EINSCHALTVORGÄNGE KANN ES BEIM STARTEN DES SEQUENZERS DAZU KOMMEN, DAS EIN PAAR KANÄLE (Relais) ANZIEHEN
// deshalb:
// Sicherheit geht vor!!!!!!
// Den Brückenschaltkreis (Zündgreis) erst scharf schalten, wenn die Anlage eingeschaltet wurde. Nicht vorher!
//
// Also:
// Zu Beginn ALLE Schalter auf AUS
// 1. Energieversorgung an die Schaltung (USB Powerbank)
// 2. Batterie   (6V an den Zündkreis
// 3. System einschalten
// 4. Testlauf ohne eingeschaltetem Zündkreis, also nur den Programmschalter auf "Lauf"
// 5. Relais auf An
// 6. Zündkreis auf An
// 7. Ab geht die Luzi! (Programschalter auf Lauf)
// 
//
// für das durchtesten der Sequenzen kann der Durchlauf ohne geschaltete Relais laufen.
//
// Wenn der Startschalter von High auf Low zurückgeschaltet wird, so werde die Sequenzen wieder von vorne beginnen (acuch bei "EndE")
//
// sequenzen in Sekunden im Array 
// sequenz n0 ist vor dem erste Zünden, am besten auf 60 sekunden oder höher 
// sequenz mit -1: Durchlauf ist zu ende, die folgenden Kanäle werden nicht geschaltet 
// es gibt keine maxiale wartezeit an sekunden
/*
* Für die Programmierung die ENTER Taste drücken
* Mit links und rechts durch die Kanäle wechseln
* mit hoch und runter die Sekunden einstellen
* mit Enter wieder raus->
* -> mit links oder rechts: speicher Ja oder Nein
*/
// werte weden seit version 1.1 aus dem eepro ausgelesneund gespeichert!
// Sequenz:                  0    1    2    3    4    5    6    7    8    9   10   11   12   13   14   15   16   17   18   19   20   21   22   23   24  
// int sequenzenZeit[25]={  60,  32,  45, 124,  59,  62,  23,  41,  43,  27,  65,  35, 125, 105,  16,  16,  5,   10,  -1,  -1,  16,  16,  16,  16,  16};

// Start wartet 60 Sekunden, dann wird 1 gezündet, 3 Sekunden Pause, 2 Zünden, 3 Sekunden Pause, 3 Zünden, 3 Sekunden Pause, 4 Zünden, 90 sekunden Pause....
//******************************************************************************************************************************************************
//    Sequenz:            0   1    2    3    4    5    6    7    8    9   10   11   12   13   14   15   16   17   18   19   20   21   22   23   24  
int sequenzenZeit[25]={  60, 32,  45, 124,  59,  62,  23,  41,  43,  27,  65,  35, 125, 105,  16,  16,   5,  10,  -1,  -1,  16,  16,  16,  16,  16   }; //sequenz feuerwrk 2022/23
//******************************************************************************************************************************************************

//Pins für die 3 Schieberegister der Zündkanäle (hintereinandergeschaltet)
//Pin connected to ST_CP of 74HC595
int latchPin = 8;
//Pin connected to SH_CP of 74HC595
int clockPin = 12;
////Pin connected to DS of 74HC595
int dataPin = 11;

// pins für die 7 Segemntanzeige des einzelnen Schieberegisters
int latchPin2=2;
int clockPin2=3;
int dataPin2=4;

// pin des Schalter, der abfragt, ob das Systm die sequenzen durchlaufen lassen soll (HIGH)
//  oder in wartesteung (LOW)
int schalterPin=5;

//frei: 6,7,9,10
//A0, A1,A2,A3,A4,A5

#define TatsenPin A1

#define MuxPin1 6
#define MuxPin2 7
#define MuxPin3 9
#define MuxPin4 10

#define MuxRead1 A0
#define MuxRead2 A2


bool sleep=true; //System wartet bei true
bool start=false; // hilfsflag für den Start

//Parameter Animation
long wartezeitAnimation=0;
long wartezeitAnimationMax=200; //alle 0,5 Sekunden wechsel der Animation
long wartezeitAnimationMaxWart=80;
int  animationStep=0;

//parameter Countdownanzeige
long wartezeitCountDown=0;
long wartezeitCountDownMax=500;
int aktuellercountdown=0;

//parameter nächster Kanal Anzeige
long kanalwartezeit=0;
long kanalwartezeitMax=1000;
int kanalAnimationStep=0; 

// zähler für die aktuelle Sequenz, beginnt bei Null, um eine Warteezeit für dem Starten zu haben, nachdem der Schalter auf HIGH betätigt wurde
int aktuelleSequenz=0;
// parameter für die noch zu wartetende zeit bis zur nächsten sequenz
long wartezeitSequenzen=0;
// hilfsparameter, um nicht immer wieder das Reials anzuziehen bzw das Schieberegister neu zu beschreiben
bool geschaltet=false; // schalter "LAuf" auf lauf bei TRUE

// die dezimalwerte des Schieberegisters für jeden einzelnen Kanal
int kanalbyte[9]={0,1,2,4,8,16,32,64,128};

// die Dezimalwerte, um die Siegensegmenteanzeige mit ziffern und Buchstaben zu füttern
int ziffern[11]={175, 130, 31, 151, 178, 181, 189, 134, 191, 183, 64}; //10=Punkt. Codes für die Ziffern auf 7 Segment
int buchstabe[5]={64, 190, 185, 45, 155 }; // 0 und a bis d // codes für die Buchstaben auf 7 Segment

bool systemzuende=false; //wenn true, sind alle sequenzen durch 

//LCD Init
jm_LCM2004A_I2C lcd(0x23); //adresse 23 des LCD

int tastenummer = 0; //gedrückte taste als nummer für die programmierung

bool programmlauf=false; // Sequenzen laufen (true) oder steht auf warten (false)
bool editieren=false; //editiermodus an oder aus
int aktuellerkanal=1;
int aktuellerkanalwert=0;

bool sollspeichern=false; //bei true daten speichern, bei nein wieder aus dem Speicher nachladen
bool fragespeichern=false; //ist die Frage nach Speichern aktiv?
bool sequenzerschirmAn=false; // der LCD zeigt die sequenzen mit dem CountDown bei True
bool istzuendeLCD_AnzeigeAn=false; //wenn true, wird der Bilschirm Programm-ENDE angezeigt

String kanalname[25] = { "Start", "A1", "A2", "A3", "A4", "A5", "A6", "B1", "B2", "B3", "B4", "B5", "B6", "C1", "C2", "C3", "C4", "C5", "C6", "D1", "D2", "D3", "D4", "D5", "D6"}; //kanalbezeichungen für die LCD ANzeige

// Menüsteuerung
int aktuellerMenuepunkt=0;
bool menueIstAktiv=false;

// vars für messungen der Kanöle
bool messenaktiv=false;
long letztemessungZeitpunkt=0;

void setup() 
{
  //Start Serial for debuging purposes
  //Serial.begin(9600);

  //Pins Initialisieren
  pinMode(latchPin, OUTPUT); // 3 Register für die 24 Kanäle
  pinMode(latchPin2, OUTPUT); // register für die 7 Segementanzeige

  pinMode(schalterPin, INPUT); // Start/Pauseschalter

  alleKanaeleaufNull(); // alles auf Null
  schreibeSiebeSegment(0); // Segmentanzeige löschen

  Wire.begin();
	lcd.begin();

  zeige_startscreen();

  //lade sequenzen aus eeprom
  loadfrom_eeprom();
  msg_ausgabe("wait for starting...");

  //Mux1
  pinMode(MuxPin1, OUTPUT); 
  pinMode(MuxPin2, OUTPUT); 
  pinMode(MuxPin3, OUTPUT); 
  pinMode(MuxPin4, OUTPUT); 

  digitalWrite(MuxPin1, LOW);
  digitalWrite(MuxPin2, LOW);
  digitalWrite(MuxPin3, LOW);
  digitalWrite(MuxPin4, LOW);

}

/*
 * bekannte hauptschleife des Arduinos
 */
void loop() 
{

  // Abfrage des Schalters für Warten oder ablaufen der Sequenzen
  // wenn der schlater low ist, dann wird geprütft, ob Enter gedrückt wurde -> dann wird das Menü aufgerufen (Programm edit, Messen)
  if (digitalRead(schalterPin) == HIGH)  // Wenn auf der Eingangsleitung des Tasters HIGH anliegt laufen die sequenzen
  {  
    //darf nur gestartet werden, wenn editmodus aus
    if (editieren==false)
    {
      sleep=false; 
      if (programmlauf==false)
      {
        msg_ausgabe("Sequenzer an!       ");
        delay(2000);
        msg_clear();
        //lcd.display_control(false, false, false);
      }
      programmlauf=true;
    }
    else
    {
      //fehler ausgeben
      msg_ausgabe("nicht erlaubt!      ");
      delay(2000);
      //lcd.display_control(true, true, true);
      msg_ausgabe("                    ");
      msg_ausgabe("wait for starting...");
    }         
  } 
  else 
  { 
    // sonstwartet das System auf den start
    sleep=true;
    programmlauf=false;
    if (sequenzerschirmAn==true)
    {
      //startbildschirm
      zeige_startscreen();
      msg_ausgabe("wait for starting...");
      sequenzerschirmAn=false;
    }
    istzuendeLCD_AnzeigeAn=false;
  }

  if (sleep==true) //system wartet auf starten des Programmes
  {
    animation();  //aufruf der Warteanimation
    systemzuende=false; // system zu ende ist auf false, um wieder von vorne beginnen zu können
    start=false;
  }
  else // sequenzen durchlaufen
  {
    if (systemzuende==false)  //sind alle 24 sequenzen durch?
    {
      //nur einmal zum start ausführen
      if (start==false)
      {
        wartezeitSequenzen=millis();
        start=true;
        aktuelleSequenz=0; // aktuelle Sequenz ist zum Start immer 0
        kanalAnimationStep=0;
      }
      sequenzenAusfuehren(); // sequenz ausführen
      zeigeCountdown(); //siebensegmentanzeige Rückmeldung der Bank/Kanal oder die letzen 9 Sekunden 
    }
    else
    {
      animationEnde();
    }
  }

  //menuetasten
  if (programmlauf==false ) //programm läuft nicht, benutzer darf ins menu
  {
    int tastenAnalogwert = analogRead(TatsenPin);
    //Serial.println(tastenAnalogwert)
    if (tastenAnalogwert<950)
    {   
      delay(350);
      getTaste(tastenAnalogwert); // welche Taset wurde gedrückt?
      //taste enter= Menüpunkt anzeigen (tastenwert=5)
      if (aktuellerMenuepunkt==0 && menueIstAktiv==false && tastenummer==5)
      {
        aktuellerMenuepunkt=1;
        editieren=false;
        menueIstAktiv=true;
        tastenummer=0;
        zeigemenue();
      }

      if (menueIstAktiv==true)
      {
        //Serial.print("menueaktion:");
        if (tastenummer==3) //taste hoch
        {
          //Serial.println("hoch");
          aktuellerMenuepunkt=aktuellerMenuepunkt+1;
          if ( aktuellerMenuepunkt>2)
          {
             aktuellerMenuepunkt=1;
          }
          zeigemenue();
          tastenummer=0;
        }
        if (tastenummer==4) //taste runter
        {
          //Serial.println("runter");
          aktuellerMenuepunkt=aktuellerMenuepunkt-1;
          if (aktuellerMenuepunkt<1)
          {
            aktuellerMenuepunkt=2;
          }
          zeigemenue();
          tastenummer=0;
        }
        
      }

      //wenn enter gedrückt und mann ist im menuepunkt messen, dann messen ausführen
      if (editieren==false && tastenummer==5 && messenaktiv==false && aktuellerMenuepunkt==2)
      {
        messenaktiv=true;
        tastenummer=0;
      }

           

      if (messenaktiv==true && tastenummer==5) //messung beenden
      { 
        messenaktiv=false;
        tastenummer=0;
        menueIstAktiv=false;
        backfromfunktion();
      }

      //wenn enter gedrück und man ist imm enüepunkt "programieren" dann mache programmieren an
      if (editieren==false && tastenummer==5 && fragespeichern==false && aktuellerMenuepunkt==1 && messenaktiv==false) 
      {
        editieren=true;
        tastenummer=0;
        editKanal();
        menueIstAktiv=false;
      }

      if (editieren==true && tastenummer==5)
      {
        editieren=false;
        //speichern abfragen
        speichernfrage();
        fragespeichern=true;
        tastenummer=0;
      }
      if (editieren==true)
      {
         //Serial.println("Editmodus");        
        if (fragespeichern==false)
        {
            //Serial.println("Editmieren");
          if (tastenummer==1)
          {
            //minus Kanal
            aktuellerkanal=aktuellerkanal-1;
            if (aktuellerkanal<0)
            {
              aktuellerkanal=24;
            }
            editKanal();
          } 
          if (tastenummer==2)
          {
            //minus Kanal
            aktuellerkanal=aktuellerkanal+1;
            if (aktuellerkanal>24)
            {
              aktuellerkanal=0;
            }
            editKanal();
          }
          if (tastenummer==3)
          {
            //Serial.println("Up");
            sequenzenZeit[aktuellerkanal]=sequenzenZeit[aktuellerkanal]+1;
            if ( sequenzenZeit[aktuellerkanal]>255)
            {
            sequenzenZeit[aktuellerkanal]=0;
            }
            zeigezeitanpassung();
          }
          if (tastenummer==4)
          {
            //Serial.println("Down");
            sequenzenZeit[aktuellerkanal]=sequenzenZeit[aktuellerkanal]-1;
            if ( sequenzenZeit[aktuellerkanal]<0)
            {
            sequenzenZeit[aktuellerkanal]=255;
            }
            zeigezeitanpassung();
          }
        }   
      }  
      if (fragespeichern==true)
      {
        //Serial.println("speicherfrage");
        if (tastenummer==1)
        {
          lcd.set_cursor(0,3);
          lcd.write("Nein");
          sollspeichern=false;
        }  
        if (tastenummer==2)
        {
          lcd.set_cursor(0,3);
          lcd.write("JA   ");
          sollspeichern=true;
        } 
        if (tastenummer==5)
        {
          //Serial.println("speichern ausfurehren");
          lcd.clear(); 
          if (sollspeichern==true)
          { 
            saveto_eeprom();
          }
          else
          {
            loadfrom_eeprom();
          }
          fragespeichern=false;
          sollspeichern=false;
          editieren=false;
          backfromfunktion();
        }
      } 
    }
  }

  if (messenaktiv==true)
  {
    // kanäle durchmessen und am Display ausgeben
    kanaeleMessen();
    //Serial.print(".");
  } 

}

/*
* zurück aus dem menü zum startscree
*/
void backfromfunktion()
{
  menueIstAktiv=false;
  tastenummer=0;
  aktuellerMenuepunkt=0;
  zeige_startscreen();
  msg_ausgabe("wait for starting...");
}


/*
* alle kanäle messenund am display anzeigen
*/ 
void  kanaeleMessen()
{
  long zeit;
  bool belegt=false;
  int zeile=1;
  int spalte=0;
  //Serial.println(letztemessungZeitpunkt);
  zeit=millis()-letztemessungZeitpunkt;
  if (zeit>5000)
  {
    letztemessungZeitpunkt=millis();
   
    zeigeMessScreen();
    for(int i = 0; i < 16; i ++)
    {
      belegt=getAnschlussBelegt(readMux(i, 0));
      zeile=1;
      if (i<6)
      {
        zeile=0;
        spalte=i;
      }
      if (i>5 && i<12)
      {
        zeile=1; 
        spalte=i-6;
      }
      if (i>11 )
      {
        zeile=2; 
        spalte=i-12;
      }
      lcd.set_cursor(spalte,zeile);
      lcd.write("-");
      lcd.set_cursor(spalte,zeile);
      if (belegt==true)
      {
        lcd.write("*");
      } 
    }  
    for(int i = 0; i < 8; i ++)
    {
       if (i<2)
      {
        zeile=2;
        spalte=i+4;
      }
      if (i>1)
      {
        zeile=3;
        spalte=i-2;
      }
      belegt=getAnschlussBelegt(readMux(i, 1));
      lcd.set_cursor(spalte,zeile);
      lcd.write("-");
      lcd.set_cursor(spalte,zeile);
      if (belegt==true)
      {
        lcd.write("*");
      } 
    }
  }
}
/*
* Messbildschirm aufbauen
* (erste 8 spalten sind für die darstellung der Messung - bedeutet nichst vorhanden * bedeutet Messbrücke angeschlossen)
*/
void zeigeMessScreen()
{
  lcd.clear_display();
  lcd.set_cursor(6,0);
  lcd.write("| Messung jede");
  lcd.set_cursor(6,1);
  lcd.write("| 5 Sek...");
  lcd.set_cursor(6,2);
  lcd.write("|");
  lcd.set_cursor(6,3);
  lcd.write("| Enter=zur\365ck");

}

bool getAnschlussBelegt(int messwert)
{
  if (messwert<1000)
  {
    return true;
  }
  return false;
}

/*
* Menuepunkte anzeigen
*/
void zeigemenue()
{
  if (aktuellerMenuepunkt==1)
  {
    msg_clear();
    msg_ausgabe("Programm editieren");
  }
  if (aktuellerMenuepunkt==2)
  {
    msg_clear();
    msg_ausgabe("Kan\xE1le messen");
  }
}

/*
* Kanal des MUX auslesen
*/

int readMux(int channel, int muxnummer) 
{
  int val=0;
  int controlPin[] = {MuxPin1, MuxPin2, MuxPin3, MuxPin4}; //4 Bitts zum setzen des Mux Kanals
  int muxChannel[16][4]={
    {0,0,0,0}, //channel 0
    {1,0,0,0}, //channel 1
    {0,1,0,0}, //channel 2
    {1,1,0,0}, //channel 3
    {0,0,1,0}, //channel 4
    {1,0,1,0}, //channel 5
    {0,1,1,0}, //channel 6
    {1,1,1,0}, //channel 7
    {0,0,0,1}, //channel 8
    {1,0,0,1}, //channel 9
    {0,1,0,1}, //channel 10
    {1,1,0,1}, //channel 11
    {0,0,1,1}, //channel 12
    {1,0,1,1}, //channel 13
    {0,1,1,1}, //channel 14
    {1,1,1,1}  //channel 15
  };
  //setze alle 4 Bits 
  for(int i = 0; i < 4; i ++){
    digitalWrite(controlPin[i], muxChannel[channel][i]);
  }
  //read the value at the SIG pin
  if (muxnummer==0)
  {
    val = analogRead(MuxRead1);
  }
  if (muxnummer==1)
  {
    val = analogRead(MuxRead2);    
  }
  //Serial.println(val);
  //return the value
  return val;
}



/*
* ANzeige zur aufzeit des Programmes, Template
*/
void sequenzer_schirm()
{
  sequenzerschirmAn=true;
  lcd.clear_display();
	lcd.set_cursor(0,0);
  lcd.write("Vorsicht! Sequenzen");
	lcd.set_cursor(0,1);
	lcd.write("aktiv!");
	lcd.set_cursor(0,2);
	lcd.write("Kanal: ");  
}
/*
* Anzeige zur Laufzeit aktualisieren (Kanal, Countdown)
*/
void sequenzer_anzeige(long sekunden)
{
  if (sequenzerschirmAn==false)
  {
    sequenzer_schirm();
  }
  lcd.set_cursor(0,3);
  lcd.write("in ");
  int zeit=sekunden+1;
  lcd.print(zeit);
  lcd.write(" Sek    "); 
  lcd.set_cursor(7,2);
  int kanalzeige;
  kanalzeige=aktuelleSequenz+1;
  lcd.print(kanalname[kanalzeige]);
  lcd.write("      ");
}

/*
* Startbildschirm
*/
void zeige_startscreen()
{
  lcd.clear_display();
	lcd.set_cursor(0,0);
	lcd.write("FWZS Vers 1.1       ");
	lcd.set_cursor(0,1);
	lcd.write("(c) 2023 by Reclof  ");
  lcd.set_cursor(0,2);
  lcd.write("--------------------");
  lcd.set_cursor(19,3);
}
/*
* Programm in das Eeprom speichern
*/
void saveto_eeprom()
{
  msg_ausgabe("Save to memory...");
  for (int i=0; i<25; i++) 
  {             
    EEPROM.write(i, sequenzenZeit[i]);
  }
  delay(1000);
  msg_clear();
}
/*
* Nachricht unterste Zeile auf dem LCD ausgeben
*/
void msg_ausgabe(char* nachricht)
{
  lcd.set_cursor(0,3);
  lcd.write(nachricht);
  lcd.set_cursor(19,3);
}
/*
* LCD unterste Reihe löschen
*/
void msg_clear()
{
  lcd.set_cursor(0,3);
  lcd.write("                    ");
}
/*
* Lade Programm aus dem Eeprom
*/
void loadfrom_eeprom()
{
  msg_ausgabe("+ Read from Memory +");
  for (int i=0; i<25; i++) 
  {   
    sequenzenZeit[i] = EEPROM.read(i);  
  }
  delay(1000);
  msg_clear();
}

/*
* ANzeige LCD - Soll Programm gespeichert werden
*/
void speichernfrage()
{
  lcd.clear_display();
  lcd.set_cursor(0,0);
  lcd.write("Sollen die Anpas-");
  lcd.set_cursor(0,1);
  lcd.write("sungen gespeichert");
  lcd.set_cursor(0,2);
  lcd.write("werden?"); 
  lcd.set_cursor(0,3);
  lcd.write("JA");
  sollspeichern=true;
}

/*
* Aktuellen Kanal Zeit anzeigen wärend des Editieren
*/
void zeigezeitanpassung()
{
  lcd.set_cursor(0,2);
  lcd.print(sequenzenZeit[aktuellerkanal]);
  //Serial.println(sequenzenZeit[aktuellerkanal]);
  lcd.write(" Sekunden");
  lcd.set_cursor(0,2);
}

/*
* Kanal anzeigen für die Editierung
*/
void editKanal()
{
  //Serial.println(aktuellerkanal);
  lcd.clear_display();
  lcd.set_cursor(0,0);
  lcd.write("Laufzeit editieren,");
  lcd.set_cursor(0,1);
  if (aktuellerkanal==0) // wenn 0 dann die Wartezeitvor dem Start editieren....
  {
      lcd.write("Startwartezeit:");
  }
  else
  {
    lcd.write("Kanal ");
	  lcd.print(aktuellerkanal);
    lcd.write(" (");
    lcd.print(kanalname[aktuellerkanal]);
    lcd.write("):");
  }
  lcd.set_cursor(0,3);
  lcd.write("-> 0=Ende vom Ablauf");
  zeigezeitanpassung();
}

/*
* Tastenweert prüfen des  Analogkanal, bei anderen WIderständen ggf Werte anpassen!
*/
void getTaste(int analogwert)
{
    tastenummer=0;
    if (analogwert>885 && analogwert<920)
    {
      tastenummer=5; //enter    
    }
    if (analogwert>640 && analogwert<680)
    {
      tastenummer=3; //hoch
    }
    if (analogwert>790 && analogwert<834)
    {
      tastenummer=4; //runter
    }
    if (analogwert>455 && analogwert<480)
    {
      tastenummer=2; //rechts
    }
    if (analogwert>275 && analogwert<305)
    {
      tastenummer=1; //links     
    }
}

/*
 * Zeigt den Countdown die ltzen 9 Sekunden bis 0 runter
 */
void zeigeCountdown()
{
  long zeitSeitderletzenSchaltung;
  long wartezeit2;
  long zeitanzeige;
  wartezeit2=sequenzenZeit[aktuelleSequenz]; 
  wartezeit2=wartezeit2*1000;
  zeitSeitderletzenSchaltung=millis()-wartezeitSequenzen;

  zeitanzeige=wartezeit2-zeitSeitderletzenSchaltung;
  zeitanzeige=zeitanzeige/1000;
  //Serial.println(zeitanzeige);
  sequenzer_anzeige(zeitanzeige);
  if (zeitanzeige<9) //die letzen 9 Sekunde anzeigen
  {
    kanalAnimationStep=0;  //kanal Anzeige auf beginn
    if (aktuellercountdown!=zeitanzeige)
    {
      zeitanzeige++;
      schreibeZiffer(ziffern[zeitanzeige]);
      aktuellercountdown=zeitanzeige;
    }
  }
  else
  {
    kanalanzeige(); // wenn über 9 Sekunden dann zeige Bank und Kanal
  }
  //countdown auf dem LCD anzeigen  
  //sequenzer_anzeige(zeitanzeige)
  
}

/*
 * zeigt Bank und Kanal auf der 7segment anzeige
 */
void kanalanzeige()
{
  long zeit;
  zeit=millis()-kanalwartezeit;
 
  if (zeit>kanalwartezeitMax)
  {
    kanalAnimationStep++;
    kanalwartezeit=millis();
  }

  int kanal=aktuelleSequenz;
  int bank=1;
  if (aktuelleSequenz>=6 && aktuelleSequenz<=13)
  {
    kanal=aktuelleSequenz-6;
    bank=2;
  }
   if (aktuelleSequenz>=12 && aktuelleSequenz<=18)
  {
    kanal=aktuelleSequenz-12;
    bank=3;
  }
  if (aktuelleSequenz>=18 && aktuelleSequenz<=24)
  {
    kanal=aktuelleSequenz-18;
    bank=4;
  }
  kanal++;
 // Serial.print("Bank:");
//  Serial.print(bank);
 // Serial.print("Kanal");
//  Serial.println(kanal);
  switch (kanalAnimationStep) 
  {
    case 0:
       schreibeSiebeSegment(0);
      break;
    case 1:
      schreibeSiebeSegment(buchstabe[bank]); //bank A B oder C
      break;
    case 2:
      schreibeSiebeSegment(ziffern[kanal]); // 1 bis 8
      break;
    case 3:
      //schreibeSiebeSegment(64); //punkt
      schreibeSiebeSegment(0);
      break;
    case 4:
      schreibeSiebeSegment(0);
      break;
  }
  if (kanalAnimationStep>2)
  {
    kanalAnimationStep=0;
  }
}
/*
 * Countdown ermitteln und entsprechenden Kanal zübnde, wenn die Zeit abgeaufen ist
 */

void sequenzenAusfuehren()
{
  long zeit;
  long wartezeit=0;
  int bank=1;
  int kanal=0;
  
  zeit=millis()-wartezeitSequenzen;

  wartezeit=sequenzenZeit[aktuelleSequenz];
  wartezeit=wartezeit*1000;
 // Serial.print("aktuelle Sequenz:");
 // Serial.print(aktuelleSequenz);
 // Serial.print(" Countdownzeit für diese Sequenz:");
 // Serial.print(sequenzenZeit[aktuelleSequenz]);
 // Serial.print(" Wartezeit:");
 // Serial.print(wartezeit);
 // Serial.print(" zeit:");
 // Serial.print(zeit);
 
 // Serial.println("");
  
  if (zeit>wartezeit) 
  {
    aktuelleSequenz++;
    wartezeitSequenzen=millis();
    geschaltet=false;
    if (aktuelleSequenz>24)
    {
      sleep=true;
    }
  }
  //richtige Bank ermitteln
  if (aktuelleSequenz>=1 && aktuelleSequenz<=8)
  {
    bank=1;
    kanal=kanalbyte[aktuelleSequenz];
  }
  if (aktuelleSequenz>=9 && aktuelleSequenz<=16)
  {
    bank=2;
    kanal=kanalbyte[aktuelleSequenz-8];
  }
  if (aktuelleSequenz>=17 && aktuelleSequenz<=24)
  {
    bank=3;
    kanal=kanalbyte[aktuelleSequenz-16];
  }
  // bank ermittel zu Ende
  
   switch (bank) //welche Bank sol geschaltet werden ? 1, 2 oder 3
   {
    case 1:
       if (geschaltet==false)
       {
         digitalWrite(latchPin, 0);
        //pin s17-24
         shiftOut(dataPin, clockPin, 0);
         //pins 9-16
         shiftOut(dataPin, clockPin, 0);
         //pin 1-8
         shiftOut(dataPin, clockPin, kanal);
         digitalWrite(latchPin, 1);
         schreibeSiebeSegment(64);
         delay(600);
         alleKanaeleaufNull();
         geschaltet=true;
       }
       break;
    case 2:
      if (geschaltet==false)
      {
        digitalWrite(latchPin, 0);
        //pin s17-24
        shiftOut(dataPin, clockPin, 0);
        //pins 9-16
        shiftOut(dataPin, clockPin, kanal);
        //pin 1-8
        shiftOut(dataPin, clockPin, 0);
        digitalWrite(latchPin, 1);
        schreibeSiebeSegment(64); 
        delay(600);
        alleKanaeleaufNull();
        geschaltet=true;
      }
      break;
    case 3:
      if (geschaltet==false)
      {
        digitalWrite(latchPin, 0);
        //pin s17-24
        shiftOut(dataPin, clockPin, kanal);
        //pins 9-16
        shiftOut(dataPin, clockPin, 0);
        //pin 1-8
        shiftOut(dataPin, clockPin, 0);
        digitalWrite(latchPin, 1);
        schreibeSiebeSegment(64); 
        delay(600);
        alleKanaeleaufNull();
        geschaltet=true;
      }
      break;
   }
   if (aktuelleSequenz>=24 || sequenzenZeit[aktuelleSequenz]==0)
   {
     // sequenzen sind durchgelaufen, system halt.
     systemzuende=true;  
   }
}

/*
 * Alle Kanäle auf NUll setzen
 */
void alleKanaeleaufNull()
{
  digitalWrite(latchPin, 0);
  shiftOut(dataPin, clockPin, 0);
  shiftOut(dataPin, clockPin, 0);
  shiftOut(dataPin, clockPin, 0);
  digitalWrite(latchPin, 1);
}

/*
 * 7-segmentAnzeige beschreiben
 */
void schreibeZiffer(int wert)
{
  digitalWrite(latchPin2, 0);
  shiftOut(dataPin2, clockPin2, wert); 
  digitalWrite(latchPin2, 1);
}

/* 
 * schieberegister beschreiben 
 */
void shiftOut(int myDataPin, int myClockPin, byte myDataOut) 
{
  // This shifts 8 bits out MSB first,
  //on the rising edge of the clock,
  //clock idles low
  //internal function setup
  int i=0;
  int pinState;
  pinMode(myClockPin, OUTPUT);
  pinMode(myDataPin, OUTPUT);
  //clear everything out just in case to
  //prepare shift register for bit shifting
  digitalWrite(myDataPin, 0);
  digitalWrite(myClockPin, 0);
  //for each bit in the byte myDataOut&#xFFFD;
  //NOTICE THAT WE ARE COUNTING DOWN in our for loop
  //This means that %00000001 or "1" will go through such
  //that it will be pin Q0 that lights.
  for (i=7; i>=0; i--)  
  {
    digitalWrite(myClockPin, 0);
    //if the value passed to myDataOut and a bitmask result
    // true then... so if we are at i=6 and our value is
    // %11010100 it would the code compares it to %01000000
    // and proceeds to set pinState to 1.
    if ( myDataOut & (1<<i) ) 
    {
      pinState= 1;
    }
    else 
    {
      pinState= 0;
    }
    //Sets the pin to HIGH or LOW depending on pinState
    digitalWrite(myDataPin, pinState);
    //register shifts bits on upstroke of clock pin
    digitalWrite(myClockPin, 1);
    //zero the data pin after shift to prevent bleed through
    digitalWrite(myDataPin, 0);
  }
  //stop shifting
  digitalWrite(myClockPin, 0);
}


/*
 * Warteanimation auf der 7 segment anzeige Kreiselnde segmente
 */
void animation() 
{
  long zeit;
  int stepold=animationStep;

  zeit=millis()-wartezeitAnimation;
  if (zeit>wartezeitAnimationMaxWart)
  {
    animationStep++;
    wartezeitAnimation=millis();
  }
  if (stepold!=animationStep)
  {
    switch (animationStep) {
      case 1:
        schreibeSiebeSegment(4);
        break;
      case 2:
        schreibeSiebeSegment(2);
        break;
      case 3:
        schreibeSiebeSegment(128);
        break;
      case 4:
        schreibeSiebeSegment(1);
        break;
      case 5:
        schreibeSiebeSegment(8);
        break;
      case 6:
        schreibeSiebeSegment(32);
        break;
    }
  }
  if (animationStep>5)
  {
    animationStep=0;
  }
}

/*
 * schreibe wert auf das Schieberegister der 7segmentanzeige
 */
void schreibeSiebeSegment(int wert)
{
  digitalWrite(latchPin2, 0);
  shiftOut(dataPin2, clockPin2, wert); 
  digitalWrite(latchPin2, 1);
}

/*
das Word Ende auf dem 7-Segment ausgeben
*/
void animationEnde()
{

  if (istzuendeLCD_AnzeigeAn==false)
  {
    istzuendeLCD_AnzeigeAn=true;
    lcd.clear_display();
    lcd.set_cursor(0,0);
    lcd.write("********************");
    lcd.set_cursor(0,1);
    lcd.write("* Sequenzer Ende.  *");
    lcd.set_cursor(0,2);
    lcd.write("* schalte auf Stop *");
    lcd.set_cursor(0,3);
    lcd.write("********************");
  }
  long zeit;
  zeit=millis()-wartezeitAnimation;
 
  if (zeit>wartezeitAnimationMax)
  {
    animationStep++;
    wartezeitAnimation=millis();
  }
  switch (animationStep) {
    case 1:
      schreibeSiebeSegment(61);
      break;
    case 2:
      schreibeSiebeSegment(152);
      break;
    case 3:
      schreibeSiebeSegment(155);
      break;
    case 4:
      schreibeSiebeSegment(61);
      break;
    case 5:
      schreibeSiebeSegment(0);
      break;
  }
  if (animationStep>8)
  {
    animationStep=0;
  }
}
