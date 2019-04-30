// Declaration des includes
#include <LiquidCrystal.h>  // LCD
#include <Wire.h>           // RTC
#include <RTClib.h>         // RTC

// Declaration des constantes
const byte pinLedRouge=2, pinLedVerte=3;
const byte pinBuzzer=22;
const byte pinBoutonPorte=12, pinBoutonMode=11, pinPhotoCell=0;

// Declaration des variables globales
byte mode; // 1: Mode bouton, 2: Mode Luminosité, 3: Mode horaire
bool etatBouton, dernierEtatBouton, etatBoutonMode, dernierEtatBoutonMode;
bool etatPorte;
int etatPhotoCell;
DateTime heureOuverture, heureFermeture;

// Declaration du LCD (numero de pin)
LiquidCrystal lcd(38, 39, 40, 41, 42, 43);

// Declaration de l'horloge
RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

void setup() {

  // Demarrage du port serie
  Serial.begin(9600);

  // Demarrage de l'horloge
  rtc.begin();
  // Decommenter la ligne suivante pour initialiser l'horloge a la date de la compilation
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  // Initialisation des heures d'ouverture et de fermeture
  heureOuverture = DateTime(2019, 04, 30, 7, 00, 00);
  heureFermeture = DateTime(2019, 04, 30, 20, 00, 00);

  // Initialisation des variables
  mode=1;
  etatBouton=LOW;
  dernierEtatBouton=HIGH;
  etatBoutonMode=HIGH;
  dernierEtatBoutonMode=HIGH;
  etatPorte=LOW;

  // Initialisation du mode des PIN
  pinMode(pinLedRouge,OUTPUT);
  pinMode(pinLedVerte,OUTPUT);
  pinMode(pinBuzzer,OUTPUT);
  pinMode(pinBoutonPorte,INPUT_PULLUP);
  pinMode(pinBoutonMode,INPUT_PULLUP);

  // Initialisation de l'etat des PIN
  digitalWrite(pinBuzzer,HIGH);

  // Initialisation du LCD
  lcd.begin(16, 2);

}

void loop() {

  // Caclul du mode
  mode=calculMode();

  // Selection du mode
  switch (mode)
  {
    case 1: // Mode 1 : Bouton
      modeBouton();
    break;
    case 2: // Mode 2 : Luminosite
      modeLuminosite();
    break;
    case 3: // Mode 3 : Horaire
      modeHorraire();
    break;
    default:

    break;
  }

  delay(100);

}

// Fonction : Lecture du bouton mode
// Return : mode
byte calculMode() {

  // Lecture du bouton
  etatBoutonMode=digitalRead(pinBoutonMode);

  // Gestion du bouton
  if(etatBoutonMode!=dernierEtatBoutonMode && etatBoutonMode==LOW) {
    buzz(100);
    mode++;
    if (mode > 3)
      mode=1;
  }

  // Mise en memoire du dernier etat du bouton
  dernierEtatBoutonMode=etatBoutonMode;

  return mode;

}

// Fonction : Mode 1 : Bouton
void modeBouton() {

  // Lecture du bouton
  etatBouton=digitalRead(pinBoutonPorte);

  // Mise a jour du LCD
  lcd.setCursor(0, 0);
  lcd.print("Mode manu              ");
  lcd.setCursor(0, 1);
  lcd.print(etatBouton);
  lcd.print("                   ");

  // Action sur la porte
  if(etatBouton!=dernierEtatBouton && etatBouton==LOW) {
    ouverturePorte(!etatPorte);
  }

  // Mise en memoire du dernier etat du bouton
  dernierEtatBouton=etatBouton;

}

// Fonction : Mode 2 : Luminosite
void modeLuminosite() {

  // Lecture de la photocell
  etatPhotoCell=analogRead(pinPhotoCell);

  // Mise a jour du LCD
  lcd.setCursor(0, 0);
  lcd.print("Mode auto :           ");
  lcd.setCursor(0, 1);
  lcd.print(etatPhotoCell);
  lcd.print("                  ");

  // Action sur la porte
  if(etatPhotoCell>500)
    ouverturePorte(true);
  else
    ouverturePorte(false);

  delay(200);

}

// Fonction : Mode 3 : Horaire
void modeHorraire() {

  // Aquisition de l'heure actuelle
  DateTime now = rtc.now();

  // Mise a jour du LCD
  lcd.setCursor(0, 0);
  lcd.print("Mode horaire : ");
  lcd.print(now.hour(), DEC);
  lcd.print(":");
  lcd.print(now.minute(), DEC);

  lcd.setCursor(0, 1);
  lcd.print("ouv:");
  lcd.print(heureOuverture.hour(), DEC);
  lcd.print(':');
  lcd.print(heureOuverture.minute(), DEC);

  lcd.print(" fer:");
  lcd.print(heureFermeture.hour(), DEC);
  lcd.print(":");
  lcd.print(heureFermeture.minute(), DEC);

  // Action sur la porte
  if(now.hour()==heureOuverture.hour() && now.minute()==heureOuverture.minute())
    ouverturePorte(true);
  else if (now.hour()==heureFermeture.hour() && now.minute()==heureFermeture.minute())
    ouverturePorte(false);

}

// Fonction déplacement de la porte (Juste des leds pour simuler pour l'instant)
// Param : Bool 1: action d'ouvrir la porteOuvert
//              2: action de fermer la porte
// Return : état de la porte
bool ouverturePorte(bool ouvrir) {
  if(!etatPorte && ouvrir) {
    digitalWrite(pinLedRouge,HIGH);
    digitalWrite(pinLedVerte,LOW);
    buzz(100);
    etatPorte=true;
    return etatPorte;
  }
  else if (etatPorte && !ouvrir) {
    digitalWrite(pinLedRouge,LOW);
    digitalWrite(pinLedVerte,HIGH);
    buzz(100);
    etatPorte=false;
    return etatPorte;
  }
  else
    return 0;
}

// Fonction faire un BIP sonore
// Param : char duree du bip
void buzz(char duree) {
  digitalWrite(pinBuzzer,LOW);
  delay(duree);
  digitalWrite(pinBuzzer,HIGH);
}
