// Declaration des includes
#include <LiquidCrystal.h>  // LCD
#include <Wire.h>           // RTC
#include <RTClib.h>         // RTC
#include <SimpleDHT.h>      // Temp + Humidity
#include <TimeLord.h>       // Sunrise-sunset
#include <Time.h>

// Declaration des constantes
const byte pinLedRouge=2, pinLedVerte=3;
const byte pinBuzzer=22;
const byte pinBoutonPorte=12, pinBoutonMode=11;
const byte pinPhotoCell=0;
const byte pinDHT=9;
const int seuilLuminosite=500;                                          // TODO: A definir
const long tempoLuminosite = 5000;                                      // TODO: A definir
const byte heureMatinMin = 5;
const byte heureMatinMax = 10;
const byte heureSoirMin = 16;
const byte heureSoirMax = 23;
const long tempoTemperature = 60000;
const long tempoSemaine = 604800000;
const float LONGITUDE = 7.139;
const float LATITUDE = 47.729;

// Declaration des variables globales
byte mode; // 1: Mode bouton, 2: Mode Luminosité, 3: Mode horaire
bool etatBouton, dernierEtatBouton, etatBoutonMode, dernierEtatBoutonMode;
bool etatPorte;
int etatPhotoCell;
DateTime now;
unsigned long currentMillis;
unsigned long previousMillisLuminosite, previousMillisTemperature, previousMillisSemaine;
SimpleDHT11 dht11(pinDHT);
byte temperature;
byte humidity;
TimeLord tardis;
byte heureOuverture;
byte heureFermeture;
byte sunrise;
byte sunset;

// Declaration du LCD (numero de pin)
LiquidCrystal lcd(38, 39, 40, 41, 42, 43);

// Declaration de l'horloge
RTC_DS3231 rtc;

void setup() {

  // Demarrage du port serie
  Serial.begin(9600);

  // Demarrage de l'horloge
  rtc.begin();
  if (! RTC.isrunning()) {
   RTC.adjust(DateTime(__DATE__, __TIME__));
 }
  // Decommenter la ligne suivante pour initialiser l'horloge a la date de la compilation
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  // Initialisation des variables
  mode=1;
  etatBouton=LOW;
  dernierEtatBouton=HIGH;
  etatBoutonMode=HIGH;
  dernierEtatBoutonMode=HIGH;
  etatPorte=LOW;
  previousMillisLuminosite = 0;
  previousMillisTemperature = 0;
  previousMillisSemaine = 0;

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

  // Aquisition de l'heure actuelle
  now = rtc.now();

  // Premiere aquisition de la temperature
  dht11.read(&temperature, &humidity, NULL);

  // Initialisation pour le calcul sunrise-sunset
  tardis.TimeZone(1 * 60); // tell TimeLord what timezone your RTC is synchronized to. You can ignore DST
  // as long as the RTC never changes back and forth between DST and non-DST
  tardis.Position(LATITUDE, LONGITUDE); // tell TimeLord where in the world we are

  calculSunriseSunset();

  // Buzz du demarrage
  buzz(100);

}

void loop() {

  // Maj de l'heure actuelle
  now = rtc.now();

  // Maj des millis (milliseconde depuis le demarrage)
  currentMillis = millis();

  AffichageTemperature();

  // Selection du mode
  switch (calculMode())
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

  // Pour une execution toutes les semaines
  if (currentMillis - previousMillisSemaine >= tempoSemaine) {
    previousMillisSemaine = currentMillis;
    calculSunriseSunset();
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
    previousMillisLuminosite = 0; // Pour le mode luminosite : Reinitialisation a chaque appui sur le bouton mode
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
  lcd.print("Mode manu                   ");

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
  lcd.print("Mode auto : ");
  lcd.print(etatPhotoCell);
  lcd.print("          ");

  // Action sur la porte
  // Si la luminosite est superieure a ... dans l'tempoLuminositee horraire ... pendant x, on ouvre la porte
  if(now.hour()>=heureMatinMin && now.hour()<heureMatinMax) {
    if(etatPhotoCell>seuilLuminosite && previousMillisLuminosite != 0) {
      if (currentMillis - previousMillisLuminosite >= tempoLuminosite) {
        previousMillisLuminosite = currentMillis;
        ouverturePorte(true);
      }
    }
    else {
      previousMillisLuminosite = currentMillis;
    }
  }

  if(now.hour()>=heureSoirMin && now.hour()<heureSoirMax) {
    if(etatPhotoCell<seuilLuminosite && previousMillisLuminosite != 0) {
      if (currentMillis - previousMillisLuminosite >= tempoLuminosite) {
        previousMillisLuminosite = currentMillis;
        ouverturePorte(false);
      }
    }
    else {
      previousMillisLuminosite = currentMillis;
    }
  }

}

// Fonction : Mode 3 : Horaire
void modeHorraire() {

  heureOuverture = sunrise;
  heureFermeture = sunset + 2;

  // Mise a jour du LCD
  lcd.setCursor(0, 0);
  lcd.print("Mode H : ");
  lcd.print(heureOuverture);
  lcd.print("H-");
  lcd.print(heureFermeture);
  lcd.print("H       ");

  // Action sur la porte
  if(now.hour()==heureOuverture)
    ouverturePorte(true);
  else if (now.hour()==heureFermeture)
    ouverturePorte(false);

}

// Fonction qui calcul les heures de sunrise et de sunset;
void calculSunriseSunset () {

  byte today[] = {0, 0, 12, now.day(), now.month(), now.year()}; // store today's date (at noon) in an array for TimeLord to use

  tardis.SunRise(today);
  sunrise=today[tl_hour];

  tardis.SunSet(today);
  sunset=today[tl_hour];

}

// Fonction déplacement de la porte (Juste des leds pour simuler pour l'instant)
// @Param : Bool 1: action d'ouvrir la porteOuvert
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

// Fonction : Aquisition toutes les "tempoTemperature" ms et affichage sur ligne 2 LCD
void AffichageTemperature() {

  // Aquisition de la temperature toute les minutes
  if (currentMillis - previousMillisTemperature >= tempoTemperature) {
    previousMillisTemperature = currentMillis;
    dht11.read(&temperature, &humidity, NULL);
  }

  // Affichage de la temperature et de l'humidite sur la deuxieme ligne
  lcd.setCursor(0, 1);
  lcd.print("T:");
  lcd.print((int)temperature);
  lcd.print("C");
  lcd.print("   H:");
  lcd.print((int)humidity);
  lcd.print("%      ");

}

// Fonction faire un BIP sonore
// @Param : char duree du bip
void buzz(char duree) {
  digitalWrite(pinBuzzer,LOW);
  delay(duree);
  digitalWrite(pinBuzzer,HIGH);
}
