// Declaration des includes
#include <LiquidCrystal.h>  // LCD
#include <Wire.h>           // RTC
#include <RTClib.h>         // RTC
#include <SimpleDHT.h>      // Temp + Humidity
#include <TimeLord.h>       // Sunrise-sunset
#include <Time.h>
#include <Stepper.h>        // Moteur pas a pas

// Declaration des constantes
const byte pinBacklightLCD=A1;
const byte pinStepper1=50, pinStepper2=51, pinStepper3=52, pinStepper4=53;
const byte pinLedRouge=2, pinLedVerte=3;
const byte pinBuzzer=22;
const byte pinBoutonPorte=12, pinBoutonMode=11;
const byte pinPhotoCell=0;
const byte pinDHT=9;
const int seuilLuminosite=500;                                          // TODO: A definir
const long tempoLuminosite = 5000;                                      // TODO: A definir
const byte heureMatinMin = 5;
const byte heureMatinMax = 10;
const byte heureSoirMin = 18;
const byte heureSoirMax = 23;
const long tempoMinute = 60000;  //
const long tempoSemaine = 604800000;  // Nombre de milliseconde dans une semaine
const long tempoAffichage = 60000;  // Temporisation de une minute pour eteindre l'affichage
const float LONGITUDE = 7.139;        // Longitude de Burnhaupt
const float LATITUDE = 47.729;        // Lattitude de Burnhaupt
const int offsetApresSunset = 180;    // Decalage apres le sunset en minutes
const int offsetAvantSunrise = 0;    // Decalage avant le sunrise en minutes
const float stepsPerRevolution = 2048;  // Nombre de pas par tour

// Declaration des variables globales
byte mode; // 1: Mode bouton, 2: Mode Luminosité, 3: Mode horaire
bool etatLCD; // False = LCD eteint; true = LCD allumé
bool etatBouton, dernierEtatBouton, etatBoutonMode, dernierEtatBoutonMode;
bool etatPorte;
int etatPhotoCell;
DateTime now;
unsigned long currentMillis;
unsigned long previousMillisLuminosite, previousMillisMinute, previousMillisSemaine, previousMillisAffichage;
SimpleDHT11 dht11(pinDHT);
byte temperature;
byte humidity;
TimeLord tardis;
int heureOuverture[2];         // Tableau de 2 cases : [heure,minute]
int heureFermeture[2];         // Tableau de 2 cases : [heure,minute]

// Declaration du stepper
Stepper myStepper(stepsPerRevolution, pinStepper4, pinStepper2, pinStepper3, pinStepper1);

// Declaration du LCD (numero de pin)
LiquidCrystal lcd(38, 39, 40, 41, 42, 43);

// Declaration de l'horloge
RTC_DS3231 rtc;

void setup() {

  // Demarrage du port serie
  Serial.begin(9600);

  // Demarrage de l'horloge
  rtc.begin();
  // Decommenter la ligne suivante pour initialiser l'horloge a la date de la compilation
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  // Initialisation des variables
  mode=1;
  etatLCD=true;
  etatBouton=LOW;
  dernierEtatBouton=HIGH;
  etatBoutonMode=HIGH;
  dernierEtatBoutonMode=HIGH;
  etatPorte=LOW;
  previousMillisLuminosite = 0;
  previousMillisMinute = 0;
  previousMillisSemaine = 0;
  previousMillisAffichage = 0;

  // Initialisation du mode des PIN
  pinMode(pinLedRouge,OUTPUT);
  pinMode(pinLedVerte,OUTPUT);
  pinMode(pinBuzzer,OUTPUT);
  pinMode(pinBoutonPorte,INPUT_PULLUP);
  pinMode(pinBoutonMode,INPUT_PULLUP);
  pinMode(pinBacklightLCD, OUTPUT);

  // Initialisation de l'etat des PIN
  digitalWrite(pinBuzzer,HIGH);

  // Initialisation du Stepper
  digitalWrite(pinStepper1,LOW);
  digitalWrite(pinStepper2,LOW);
  digitalWrite(pinStepper3,LOW);
  digitalWrite(pinStepper4,LOW);

  // Allumage du backlight
  digitalWrite(pinBacklightLCD, HIGH);

  // Initialisation du LCD
  lcd.begin(16, 2);

  // Aquisition de l'heure actuelle
  now = rtc.now();

  // Initialisation pour le calcul sunrise-sunset
  tardis.TimeZone(1 * 60); // tell TimeLord what timezone your RTC is synchronized to. You can ignore DST
  // as long as the RTC never changes back and forth between DST and non-DST
  tardis.Position(LATITUDE, LONGITUDE); // tell TimeLord where in the world we are

  // Execution de fonctions pour initialisation
  calculSunriseSunset();
  dht11.read(&temperature, &humidity, NULL);
  ouverturePorte(true);

  // Buzz du demarrage
  buzz(100);

}

void loop() {

  // Maj de l'heure actuelle
  now = rtc.now();

  // Maj des millis (milliseconde depuis le demarrage)
  currentMillis = millis();

  // Maj de l'affichage
  AffichageLCD();

  // Selection du mode
  switch (mode = calculMode())
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

  // Execution toutes les minutes
  if (currentMillis - previousMillisMinute >= tempoMinute) {
    previousMillisMinute = currentMillis;

    dht11.read(&temperature, &humidity, NULL);
  }

  // Execution toutes les semaines
  if (currentMillis - previousMillisSemaine >= tempoSemaine) {
    previousMillisSemaine = currentMillis;

    calculSunriseSunset();
  }

  // Execution toutes les minutes pour éteindre le LCD
  if (currentMillis - previousMillisAffichage >= tempoAffichage) {

    Serial.println(currentMillis);
    Serial.println(previousMillisAffichage);
    Serial.println(etatLCD);

    previousMillisAffichage = currentMillis;

    // On eteint le lcd
    lcd.noDisplay();
    digitalWrite(pinBacklightLCD, LOW);
    etatLCD=false;
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
    previousMillisAffichage = currentMillis; // A chaque appui sur le bouton, on recommence a compter

    // Si le LCD est eteint, on l'allume, sinon on change de mode
    if (!etatLCD) {
      // Rallumage du lcd
      lcd.display();
      digitalWrite(pinBacklightLCD, HIGH);
      etatLCD=true;
    }
    else {
      mode++;
      if (mode > 3)
        mode=1;
    }
  }

  // Mise en memoire du dernier etat du bouton
  dernierEtatBoutonMode=etatBoutonMode;

  return mode;

}

// Fonction : Mode 1 : Bouton
void modeBouton() {

  // Lecture du bouton
  etatBouton=digitalRead(pinBoutonPorte);

  // Action sur la porte
  if(etatBouton!=dernierEtatBouton && etatBouton==LOW) {

    buzz(100);
    previousMillisAffichage = currentMillis; // A chaque appui sur le bouton, on recommence a compter

    // Si le lcd est eteint, on l'allume, sinon on actionne la porte
    if (!etatLCD) {
      // Rallumage du lcd
      lcd.display();
      digitalWrite(pinBacklightLCD, HIGH);
      etatLCD=true;
    }
    else {
      ouverturePorte(!etatPorte);
    }
  }

  // Mise en memoire du dernier etat du bouton
  dernierEtatBouton=etatBouton;

}

// Fonction : Mode 2 : Luminosite
void modeLuminosite() {

  // Lecture de la photocell
  etatPhotoCell=analogRead(pinPhotoCell);

  // Action sur la porte
  // Si la luminosite est superieure a ... dans la plage horraire horraire ... pendant x, on ouvre la porte
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

  // Securite : ouverture/fermeture de la porte aux horaires max du matin/soir
  if(now.hour()==heureMatinMax)
    ouverturePorte(true);
  if(now.hour()==heureSoirMax)
    ouverturePorte(false);

}

// Fonction : Mode 3 : Horaire
void modeHorraire() {

  // Action sur la porte
  if(now.hour()==heureOuverture[0] && now.minute()==heureOuverture[1])
    ouverturePorte(true);
  else if (now.hour()==heureFermeture[0] && now.minute()==heureFermeture[1])
    ouverturePorte(false);

}

// Fonction qui calcul les heures de sunrise et de sunset, et initialisation des variable heureOuverture et heureFermeture;
void calculSunriseSunset () {

  byte today[] = {0, 0, 12, now.day(), now.month(), now.year()}; // store today's date (at noon) in an array for TimeLord to use

  // Calcul du Sunrise et affectation des variables heureOuverture
  tardis.SunRise(today);
  heureOuverture[0] = today[tl_hour];
  heureOuverture[1] = today[tl_minute] - offsetAvantSunrise;

  while (heureOuverture[1] < 0) {
    heureOuverture[0] = heureOuverture[0] - 1;
    heureOuverture[1] = heureOuverture[1] + 60;
  }

  // Calcul du Sunset et affectation des variables heureFermeture
  tardis.SunSet(today);

  heureFermeture[0] = today[tl_hour];
  heureFermeture[1] = today[tl_minute] + offsetApresSunset;

  while (heureFermeture[1] > 59) {
    heureFermeture[0] = heureFermeture[0] + 1;
    heureFermeture[1] = heureFermeture[1] - 60;
  }

}

// Fonction déplacement de la porte (Juste des leds pour simuler pour l'instant)
// @Param : Bool 1: action d'ouvrir la porteOuvert
//              2: action de fermer la porte
// Return : état de la porte
bool ouverturePorte(bool ouvrir) {
  if(!etatPorte && ouvrir) {
    digitalWrite(pinLedRouge,LOW);
    digitalWrite(pinLedVerte,LOW);
    buzz(50);

    myStepper.setSpeed(10); // En tr/min
    myStepper.step(1024);

    digitalWrite(pinLedRouge,HIGH);
    digitalWrite(pinLedVerte,LOW);
    buzz(100);

    digitalWrite(pinStepper1,LOW);
    digitalWrite(pinStepper2,LOW);
    digitalWrite(pinStepper3,LOW);
    digitalWrite(pinStepper4,LOW);

    etatPorte=true;
    return etatPorte;
  }
  else if (etatPorte && !ouvrir) {
    digitalWrite(pinLedRouge,LOW);
    digitalWrite(pinLedVerte,LOW);
    buzz(50);

    myStepper.setSpeed(10); // En tr/min
    myStepper.step(-1024);

    digitalWrite(pinLedRouge,LOW);
    digitalWrite(pinLedVerte,HIGH);
    buzz(100);

    digitalWrite(pinStepper1,LOW);
    digitalWrite(pinStepper2,LOW);
    digitalWrite(pinStepper3,LOW);
    digitalWrite(pinStepper4,LOW);

    etatPorte=false;
    return etatPorte;
  }
  else
    return 0;
}

// Fonction : Affichage sur le LCD
void AffichageLCD() {



  // Affichage de l'heure, la temperature et de l'humidite sur la premiere ligne
  lcd.setCursor(0, 0);
  lcd.print((int)now.hour(), DEC);
  lcd.print(":");
  if (now.minute() < 10) {
    lcd.print("0");
    lcd.print((int)now.minute());
  }
  else {
    lcd.print((int)now.minute());
  }
  lcd.print("  ");
  lcd.print((int)temperature);
  lcd.print("C");
  lcd.print("  ");
  lcd.print((int)humidity);
  lcd.print("%   ");

  // Affichage de la deuxieme ligne selon le mode
  lcd.setCursor(0, 1);
  switch (mode)
  {
    case 1: // Mode 1 : Bouton
      lcd.print("Mode manu                   ");
    break;
    case 2: // Mode 2 : Luminosite
      lcd.print("Mode auto : ");
      lcd.print(etatPhotoCell);
      lcd.print("          ");
    break;
    case 3: // Mode 3 : Horaire
      lcd.print(heureOuverture[0]);
      lcd.print(":");
      lcd.print(heureOuverture[1]);
      lcd.print(" - ");
      lcd.print(heureFermeture[0]);
      lcd.print(":");
      lcd.print(heureFermeture[1]);
      lcd.print("           ");
    break;
    default:
      lcd.print("Erreur");
    break;
  }

}

// Fonction faire un BIP sonore
// @Param : char duree du bip
void buzz(char duree) {
  digitalWrite(pinBuzzer,LOW);
  delay(duree);
  digitalWrite(pinBuzzer,HIGH);
}
