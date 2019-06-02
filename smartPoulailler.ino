// Declaration des includes
#include <LiquidCrystal.h>  // LCD
// #include <Wire.h>           // RTC
// #include <RTClib.h>         // RTC
#include <TimeLord.h>       // Sunrise-sunset
// #include <Time.h>
#include <avr/sleep.h>      // Mode sleep
#include <DS3232RTC.h>
#include <Stepper.h>        // Moteur pas a pas


// Define pin
#define pinBoutonFinDeCourseHaut 7
#define pinBoutonFinDeCourseBas 7 // Pour test, a remettre a 6
#define pinBacklightLCD A1
#define pinStepper1 50
#define pinStepper2 51
#define pinStepper3 52
#define pinStepper4 53
#define pinLedRouge 2
#define pinLedVerte 3
#define pinBuzzer 22
#define pinBoutonPorte 12
#define pinBoutonMode 18
#define pinPhotoCell 0
#define pinInterruptRTC 19  // Pin reliée au SQW du module RTC (Utilisée pour reveiller la carte)

// Declaration des constantes
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
const int offsetSunset = 60;    // Decalage apres le sunset en minutes (peut etre negatif)
const int offsetSunrise = -30;    // Decalage après le sunrise en minutes (peut etre negatif)
const float stepsPerRevolution = 2048;  // Nombre de pas par tour (Caracteristique du moteur)
const int stepperSpeed = 15; // en tr/min

// Declaration des variables globales
byte mode; // 1: Mode bouton, 2: Mode Luminosité, 3: Mode horaire
bool etatLCD; // False = LCD eteint; true = LCD allumé
bool etatBouton, dernierEtatBouton, etatBoutonMode, dernierEtatBoutonMode;
bool etatPorte;
int etatPhotoCell;
time_t t;
unsigned long currentMillis;
unsigned long previousMillisLuminosite, previousMillisMinute, previousMillisSemaine, previousMillisAffichage;
TimeLord tardis;
int heureOuverture[2];         // Tableau de 2 cases : [heure,minute]
int heureFermeture[2];         // Tableau de 2 cases : [heure,minute]

// Declaration du stepper
Stepper myStepper(stepsPerRevolution, pinStepper4, pinStepper2, pinStepper3, pinStepper1);

// Declaration du LCD (numero de pin)
LiquidCrystal lcd(38, 39, 40, 41, 42, 43);

// Declaration de l'horloge
// RTC_DS3231 rtc;
DS3232RTC rtc;

void setup() {

  // Demarrage du port serie
  Serial.begin(9600);

  // Demarrage de l'horloge
  rtc.begin();

  // Décommenter le bloc ci-dessous pour regler l'heure
  // tmElements_t tm;
  // tm.Second = 00;
  // tm.Minute = 17;
  // tm.Hour = 13;               // set the RTC to an arbitrary time
  // tm.Day = 2;
  // tm.Month = 6;
  // tm.Year = 2019 - 1970;      // tmElements_t.Year is the offset from 1970
  // RTC.write(tm);              // set the RTC from the tm structure

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
  pinMode(LED_BUILTIN,OUTPUT); // Utilisation de la led 13 pour indiquer le mode veille
  pinMode(pinInterruptRTC,INPUT_PULLUP);
  pinMode(pinLedRouge,OUTPUT);
  pinMode(pinLedVerte,OUTPUT);
  pinMode(pinBuzzer,OUTPUT);
  pinMode(pinBoutonPorte,INPUT_PULLUP);
  pinMode(pinBoutonMode,INPUT_PULLUP);
  pinMode(pinBoutonFinDeCourseHaut,INPUT_PULLUP);
  pinMode(pinBoutonFinDeCourseBas,INPUT_PULLUP);
  pinMode(pinBacklightLCD, OUTPUT);

  // Initialisation de l'etat des PIN
  digitalWrite(LED_BUILTIN,LOW);
  digitalWrite(pinBuzzer,HIGH);
  digitalWrite(pinLedRouge,LOW);
  digitalWrite(pinLedVerte,HIGH);

  // Initialisation du Stepper
  digitalWrite(pinStepper1,LOW);
  digitalWrite(pinStepper2,LOW);
  digitalWrite(pinStepper3,LOW);
  digitalWrite(pinStepper4,LOW);

  // Allumage du backlight
  digitalWrite(pinBacklightLCD, HIGH);

  // Initialisation du LCD
  lcd.begin(16, 2);

  // Initialsation de la vitesse du stepper
  myStepper.setSpeed(stepperSpeed);

  // Aquisition de l'heure actuelle
  // now = rtc.now();
  t = RTC.get();

  // Initialisation de l'alarme, reset des flags
  RTC.setAlarm(ALM1_MATCH_DATE, 0, 0, 0, 1);
  RTC.setAlarm(ALM2_MATCH_DATE, 0, 0, 0, 1);
  RTC.alarm(ALARM_1);
  RTC.alarm(ALARM_2);
  RTC.alarmInterrupt(ALARM_1, false);
  RTC.alarmInterrupt(ALARM_2, false);
  RTC.squareWave(SQWAVE_NONE);

  // RTC.setAlarm(alarmType, minutes, hours, dayOrDate);
  RTC.setAlarm(ALM1_MATCH_HOURS, 26, 13, 0); // Set de l'alarme 1
  RTC.alarm(ALARM_1); // clear the alarm flag
  RTC.squareWave(SQWAVE_NONE); // configure the INT/SQW pin for "interrupt" operation
  RTC.alarmInterrupt(ALARM_1, true); // enable interrupt output for Alarm 1

  // Initialisation pour le calcul sunrise-sunset
  tardis.TimeZone(1 * 60); // tell TimeLord what timezone your RTC is synchronized to. You can ignore DST
  // as long as the RTC never changes back and forth between DST and non-DST
  tardis.Position(LATITUDE, LONGITUDE); // tell TimeLord where in the world we are

  // Execution de fonctions pour initialisation
  calculSunriseSunset();

  // Buzz du demarrage
  buzz(100);

}

void loop() {

  // Maj de l'heure actuelle
  t = RTC.get();

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
  }

  // Execution toutes les semaines
  if (currentMillis - previousMillisSemaine >= tempoSemaine) {
    previousMillisSemaine = currentMillis;

    calculSunriseSunset();
  }

  // Execution toutes les minutes pour éteindre le LCD
  if (currentMillis - previousMillisAffichage >= tempoAffichage) {

    previousMillisAffichage = currentMillis;

    // On eteint le lcd
    extinctionLCD();
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
      allumageLCD();
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
  if(hour(t)>=heureMatinMin && hour(t)<heureMatinMax) {
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

  if(hour(t)>=heureSoirMin && hour(t)<heureSoirMax) {
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
  if(hour(t)==heureMatinMax)
    ouverturePorte(true);
  if(hour(t)==heureSoirMax)
    ouverturePorte(false);

}

// Fonction : Mode 3 : Horaire
void modeHorraire() {

  // Action sur la porte
  if(hour(t)==heureOuverture[0] && minute(t)==heureOuverture[1])
    ouverturePorte(true);
  else if (hour(t)==heureFermeture[0] && minute(t)==heureFermeture[1])
    ouverturePorte(false);

}

// Fonction de mise en veille
void goingToSleep() {
  sleep_enable();//Enabling sleep mode
  attachInterrupt(digitalPinToInterrupt(pinInterruptRTC), wakeUp, LOW); //attaching a interrupt to pin RTC
  attachInterrupt(digitalPinToInterrupt(pinBoutonMode), wakeUp, LOW); //attaching a interrupt to pin boutonMode
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);//Setting the sleep mode, in our case full sleep
  digitalWrite(LED_BUILTIN,HIGH);

  extinctionLCD();

  // Extinction des leds
  digitalWrite(pinLedRouge,HIGH);
  digitalWrite(pinLedVerte,HIGH);

  delay(1000);

  sleep_cpu();//activating sleep mode
  digitalWrite(LED_BUILTIN,LOW);
  RTC.setAlarm(ALM1_MATCH_HOURS, 27, 13, 0); //Set New Alarm
  RTC.alarm(ALARM_1); // clear the alarm flag
}

// Fonction éxécutée au réveil
void wakeUp() {
  sleep_disable();//Disable sleep mode
  detachInterrupt(digitalPinToInterrupt(pinInterruptRTC)); //Removes the interrupt from pin
  detachInterrupt(digitalPinToInterrupt(pinBoutonMode)); //Removes the interrupt from pin
}

// Fonction qui calcul les heures de sunrise et de sunset, et initialisation des variable heureOuverture et heureFermeture;
void calculSunriseSunset () {

  byte today[] = {0, 0, 12, day(t), month(t), year(t)}; // store today's date (at noon) in an array for TimeLord to use

  // Calcul du Sunrise et affectation des variables heureOuverture
  tardis.SunRise(today);
  heureOuverture[0] = today[tl_hour];
  heureOuverture[1] = today[tl_minute] + offsetSunrise;

  if (heureOuverture[1] < 0) {
    while (heureOuverture[1] < 0) {
      heureOuverture[0] = heureOuverture[0] - 1;
      heureOuverture[1] = heureOuverture[1] + 60;
    }
  }

  if (heureOuverture[1] > 59) {
    while (heureOuverture[1] > 59) {
      heureOuverture[0] = heureOuverture[0] + 1;
      heureOuverture[1] = heureOuverture[1] - 60;
    }
  }

  // Calcul du Sunset et affectation des variables heureFermeture
  tardis.SunSet(today);

  heureFermeture[0] = today[tl_hour];
  heureFermeture[1] = today[tl_minute] + offsetSunset;

  if (heureFermeture[1] < 0) {
    while (heureFermeture[1] < 0) {
      heureFermeture[0] = heureFermeture[0] - 1;
      heureFermeture[1] = heureFermeture[1] + 60;
    }
  }

  if (heureFermeture[1] > 59) {
    while (heureFermeture[1] > 59) {
      heureFermeture[0] = heureFermeture[0] + 1;
      heureFermeture[1] = heureFermeture[1] - 60;
    }
  }

}

// Fonction déplacement de la porte (Juste des leds pour simuler pour l'instant)
// @Param : Bool 1: action d'ouvrir la porteOuvert
//              2: action de fermer la porte
// Return : état de la porte
void ouverturePorte(bool ouvrir) {

  if(!etatPorte && ouvrir) {

    digitalWrite(pinLedRouge,LOW);
    digitalWrite(pinLedVerte,LOW);
    buzz(50);

    // Tant que le bouton de fin de course n'est pas actionnee, on fait tourner le moteur
    while (digitalRead(pinBoutonFinDeCourseHaut) != 0) {
      myStepper.step(40);
    }

    // Allumage des Leds
    digitalWrite(pinLedRouge,HIGH);
    digitalWrite(pinLedVerte,LOW);

    // Extinction du stepper motor
    digitalWrite(pinStepper1,LOW);
    digitalWrite(pinStepper2,LOW);
    digitalWrite(pinStepper3,LOW);
    digitalWrite(pinStepper4,LOW);

    buzz(100);
    etatPorte=true;

  }

  else if (etatPorte && !ouvrir) {
    digitalWrite(pinLedRouge,LOW);
    digitalWrite(pinLedVerte,LOW);
    buzz(50);

    // Tant que le bouton de fin de course n'est pas acctionnee
    while (digitalRead(pinBoutonFinDeCourseBas) != 0)
      myStepper.step(-40);

    // Allumage des Leds
    digitalWrite(pinLedRouge,LOW);
    digitalWrite(pinLedVerte,HIGH);

    // Extinction du stepper motor
    digitalWrite(pinStepper1,LOW);
    digitalWrite(pinStepper2,LOW);
    digitalWrite(pinStepper3,LOW);
    digitalWrite(pinStepper4,LOW);

    buzz(100);
    etatPorte=false;
  }

  goingToSleep();

}

// Fonction extinction du LCD et du backlight
void extinctionLCD() {
  lcd.noDisplay();
  digitalWrite(pinBacklightLCD, LOW);
  etatLCD=false;
}

// Fonction allumage du LCD et du backlight
void allumageLCD() {
  lcd.display();
  digitalWrite(pinBacklightLCD, HIGH);
  etatLCD=true;
}

// Fonction : Affichage sur le LCD
void AffichageLCD() {


  // Affichage de l'heure  sur la premiere ligne
  lcd.setCursor(0, 0);
  lcd.print((int)hour(t), DEC);
  lcd.print(":");
  if (minute(t) < 10) {
    lcd.print("0");
    lcd.print((int)minute(t));
  }
  else {
    lcd.print((int)minute(t));
  }
  lcd.print(" SmartPoule");

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
      if (heureOuverture[1] < 10) {
        lcd.print("0");
        lcd.print(heureOuverture[1]);
      }
      else {
        lcd.print(heureOuverture[1]);
      }
      lcd.print(" - ");
      lcd.print(heureFermeture[0]);
      lcd.print(":");
      if (heureFermeture[1] < 10) {
        lcd.print("0");
        lcd.print(heureFermeture[1]);
      }
      else {
        lcd.print(heureFermeture[1]);
      }
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
