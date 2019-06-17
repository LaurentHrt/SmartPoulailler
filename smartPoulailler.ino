// Declaration des includes
#include <LiquidCrystal.h>  // LCD
#include <TimeLord.h>       // Sunrise-sunset
#include <avr/sleep.h>      // Mode sleep
#include <DS3232RTC.h>      // RTC
#include <Stepper.h>        // Moteur pas a pas

// Define pin
#define pinPhotoCell A0
#define pinBacklightLCD A1
#define pinLedRouge 2
#define pinLedVerte 3
#define pinBoutonFinDeCourseHaut 7
#define pinBoutonFinDeCourseBas 7 // Pour test, a remettre a 6
#define pinBoutonPorte 12
#define pinBoutonMode 18
#define pinInterruptRTC 19  // Pin reliée au SQW du module RTC (Utilisée pour reveiller la carte)
#define pinBuzzer 22
#define pinLCD_RS 38
#define pinLCD_E 39
#define pinLCD_D4 40
#define pinLCD_D5 41
#define pinLCD_D6 42
#define pinLCD_D7 43
#define pinStepper_IN1 50
#define pinStepper_IN2 51
#define pinStepper_IN3 52
#define pinStepper_IN4 53

// Declaration des structures
struct struct_time {
  int heure;
  int minute;
};

// Declaration des constantes
const int seuilLuminosite=500;                                          // #TODO: A definir
const long tempoLuminosite = 5000;                                      // #TODO: A definir
const byte heureMatinMin = 5;
const byte heureMatinMax = 10;
const byte heureSoirMin = 18;
const byte heureSoirMax = 23;
const long tempoSleep = 60000;  // Temporisation de une minute pour eteindre l'affichage
const float LONGITUDE = 7.139;        // Longitude de Burnhaupt
const float LATITUDE = 47.729;        // Lattitude de Burnhaupt
const int offsetSunset = 200;    // Decalage apres le sunset en minutes (peut etre negatif) (Il y a un decalage de 2 heures apparemment)
const int offsetSunrise = -60;    // Decalage après le sunrise en minutes (peut etre negatif)
const float stepsPerRevolution = 2048;  // Nombre de pas par tour (Caracteristique du moteur)
const int stepperSpeed = 12; // en tr/min

// Declaration des variables globales
byte mode; // 1: Mode bouton, 2: Mode horaire, 3: Mode Luminosité
bool etatLCD; // False = LCD eteint; true = LCD allumé
bool etatBouton, dernierEtatBouton, etatBoutonMode, dernierEtatBoutonMode;
bool etatPorte;
int etatPhotoCell;
time_t t;
unsigned long currentMillis;
unsigned long previousMillisLuminosite, previousMillisSleep;
TimeLord tardis;
struct_time heureOuverture = {0, 0};
struct_time heureFermeture = {0, 0};

// Declaration du stepper
Stepper myStepper(stepsPerRevolution, pinStepper_IN4, pinStepper_IN2, pinStepper_IN3, pinStepper_IN1);

// Declaration du LCD (numero de pin)
LiquidCrystal lcd(pinLCD_RS, pinLCD_E, pinLCD_D4, pinLCD_D5, pinLCD_D6, pinLCD_D7);

// Declaration de l'horloge
DS3232RTC rtc;

void setup() {

  // Demarrage du port serie
  Serial.begin(9600);

  // Demarrage de l'horloge
  rtc.begin();

  // Décommenter le bloc ci-dessous pour regler l'heure
  // tmElements_t tm;
  // tm.Second = 00;
  // tm.Minute = 51;
  // tm.Hour = 17;               // set the RTC to an arbitrary time
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
  previousMillisSleep = 0;

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
  digitalWrite(pinStepper_IN1,LOW);
  digitalWrite(pinStepper_IN2,LOW);
  digitalWrite(pinStepper_IN3,LOW);
  digitalWrite(pinStepper_IN4,LOW);

  // Initialisation du LCD
  lcd.begin(16, 2);
  allumageLCD();

  // Initialsation de la vitesse du stepper
  myStepper.setSpeed(stepperSpeed);

  // Aquisition de l'heure actuelle
  t = RTC.get();

  // Calcul des horraires sunrise-sunset
  calculSunriseSunset();

  // Initialisation des alarmes, reset des flags
  // RTC.setAlarm(alarmType, minutes, hours, dayOrDate);
  RTC.setAlarm(ALM1_MATCH_HOURS, heureOuverture.minute, heureOuverture.heure, 0); // Set de l'alarme 1 sur l'heure de l'ouverture - offset
  RTC.setAlarm(ALM2_MATCH_HOURS, heureFermeture.minute, heureFermeture.heure, 0); // Set de l'alarme 2 sur l'heure de la fermeture - offset
  RTC.alarm(ALARM_1);
  RTC.alarm(ALARM_2);
  RTC.alarmInterrupt(ALARM_1, true); // Enable interrupt output for Alarm 1
  RTC.alarmInterrupt(ALARM_2, true); // Enable interrupt output for Alarm 2
  RTC.squareWave(SQWAVE_NONE); // Configure the INT/SQW pin for "interrupt" operation

  // Initialisation pour le calcul sunrise-sunset
  tardis.TimeZone(1 * 60); // tell TimeLord what timezone your RTC is synchronized to. You can ignore DST
  // as long as the RTC never changes back and forth between DST and non-DST
  tardis.Position(LATITUDE, LONGITUDE); // tell TimeLord where in the world we are

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
    case 2: // Mode 2 : Horaire
      modeHorraire();
    break;
    case 3: // Mode 3 : Luminosite
      modeLuminosite();
    break;
    default:

    break;
  }

  // Execution toutes les minutes pour passer en sleep
  if (currentMillis - previousMillisSleep >= tempoSleep) {
    previousMillisSleep = currentMillis;

    // On se met en veille
    goingToSleep();
  }

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
    previousMillisSleep = currentMillis; // A chaque appui sur le bouton, on recommence a compter

    // Si le LCD est eteint, on l'allume, sinon on change de mode

      mode++;
      if (mode > 2)  // TEMPORAIRE On ne passe pas dans le mode luminosité (non fonctionnel)
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

  // Action sur la porte
  if(etatBouton!=dernierEtatBouton && etatBouton==LOW) {

    buzz(100);
    previousMillisSleep = currentMillis; // A chaque appui sur le bouton, on recommence a compter

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

// Fonction : Mode 2 : Horaire
void modeHorraire() {

  // Action sur la porte
  if(hour(t)==heureOuverture.heure && minute(t)==heureOuverture.minute)
    ouverturePorte(true);
  else if (hour(t)==heureFermeture.heure && minute(t)==heureFermeture.minute)
    ouverturePorte(false);

}

// Fonction de mise en veille
void goingToSleep() {
  sleep_enable();//Enabling sleep mode
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); //Setting the sleep mode, in our case full sleep

  attachInterrupt(digitalPinToInterrupt(pinInterruptRTC), wakeUp, LOW); //attaching a interrupt to pin RTC
  attachInterrupt(digitalPinToInterrupt(pinBoutonMode), wakeUp, LOW); //attaching a interrupt to pin boutonMode

  delay(1000);

  extinctionLCD();

  // Extinction des leds, allumage de la led 13
  digitalWrite(LED_BUILTIN,HIGH);
  // digitalWrite(pinLedRouge,HIGH);
  // digitalWrite(pinLedVerte,HIGH);

  delay(1000);

  sleep_cpu(); //activating sleep mode

  // Delai pour pas que l'appui soit pris en compte pour changer de mode
  delay(2000);

  // Set new alarms
  RTC.setAlarm(ALM1_MATCH_HOURS, heureOuverture.minute, heureOuverture.heure, 0); // Set de l'alarme 1 sur l'heure de l'ouverture - offset
  RTC.setAlarm(ALM2_MATCH_HOURS, heureFermeture.minute, heureFermeture.heure, 0); // Set de l'alarme 2 sur l'heure de la fermeture - offset

  // Clear the alarm flag
  RTC.alarm(ALARM_1);
  RTC.alarm(ALARM_2);
}

// Fonction : Mode 3 : Luminosite
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

// Fonction éxécutée au réveil
void wakeUp() {
  // Extinction de la led 13
  digitalWrite(LED_BUILTIN,LOW);

  allumageLCD();

  calculSunriseSunset();

  previousMillisSleep = currentMillis;

  sleep_disable();//Disable sleep mode
  detachInterrupt(digitalPinToInterrupt(pinInterruptRTC)); //Removes the interrupt from pin
  detachInterrupt(digitalPinToInterrupt(pinBoutonMode)); //Removes the interrupt from pin
}

// Fonction qui calcul les heures de sunrise et de sunset, et initialisation des variable heureOuverture et heureFermeture;
void calculSunriseSunset () {

  byte today[] = {0, 0, 12, day(t), month(t), year(t)}; // store today's date (at noon) in an array for TimeLord to use

  // Calcul du Sunrise et affectation des variables heureOuverture
  tardis.SunRise(today);

  heureOuverture.heure = today[tl_hour];
  heureOuverture.minute = today[tl_minute] + offsetSunrise;

  if (heureOuverture.minute < 0) {
    while (heureOuverture.minute < 0) {
      heureOuverture.heure = heureOuverture.heure - 1;
      heureOuverture.minute = heureOuverture.minute + 60;
    }
  }

  if (heureOuverture.minute > 59) {
    while (heureOuverture.minute > 59) {
      heureOuverture.heure = heureOuverture.heure + 1;
      heureOuverture.minute = heureOuverture.minute - 60;
    }
  }

  // Calcul du Sunset et affectation des variables heureFermeture
  tardis.SunSet(today);

  heureFermeture.heure = today[tl_hour];
  heureFermeture.minute = today[tl_minute] + offsetSunset;

  if (heureFermeture.minute < 0) {
    while (heureFermeture.minute < 0) {
      heureFermeture.heure = heureFermeture.heure - 1;
      heureFermeture.minute = heureFermeture.minute + 60;
    }
  }

  if (heureFermeture.minute > 59) {
    while (heureFermeture.minute > 59) {
      heureFermeture.heure = heureFermeture.heure + 1;
      heureFermeture.minute = heureFermeture.minute - 60;
    }
  }

}

// Fonction déplacement de la porte (Juste des leds pour simuler pour l'instant)
// @Param : Bool 1: action d'ouvrir la porteOuvert
//              2: action de fermer la porte
// Return : état de la porte
void ouverturePorte(bool ouvrir) {

  if(!etatPorte && ouvrir) {

    // On éteint tout pour garder la puissance
    extinctionLCD();
    digitalWrite(pinLedRouge,HIGH);
    digitalWrite(pinLedVerte,HIGH);
    buzz(50);

    // Tant que le bouton de fin de course n'est pas actionnee, on fait tourner le moteur
    while (digitalRead(pinBoutonFinDeCourseHaut) != 0)
      myStepper.step(40);

    // On rallume les leds et le LCD
    digitalWrite(pinLedRouge,HIGH);
    digitalWrite(pinLedVerte,LOW);

    // Extinction du stepper motor
    digitalWrite(pinStepper_IN1,LOW);
    digitalWrite(pinStepper_IN2,LOW);
    digitalWrite(pinStepper_IN3,LOW);
    digitalWrite(pinStepper_IN4,LOW);

    buzz(100);
    etatPorte=true;

    goingToSleep();

  }

  else if (etatPorte && !ouvrir) {

    // On éteint tout pour garder la puissance
    extinctionLCD();
    digitalWrite(pinLedRouge,HIGH);
    digitalWrite(pinLedVerte,HIGH);
    buzz(50);

    // Tant que le bouton de fin de course n'est pas acctionnee
    while (digitalRead(pinBoutonFinDeCourseBas) != 0)
      myStepper.step(-40);

    // Allumage des Leds
    digitalWrite(pinLedRouge,LOW);
    digitalWrite(pinLedVerte,HIGH);

    // Extinction du stepper motor
    digitalWrite(pinStepper_IN1,LOW);
    digitalWrite(pinStepper_IN2,LOW);
    digitalWrite(pinStepper_IN3,LOW);
    digitalWrite(pinStepper_IN4,LOW);

    buzz(100);
    etatPorte=false;

    goingToSleep();

  }

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
    case 2: // Mode 2 : Horaire
      lcd.print(heureOuverture.heure);
      lcd.print(":");
      if (heureOuverture.minute < 10) {
        lcd.print("0");
        lcd.print(heureOuverture.minute);
      }
      else {
        lcd.print(heureOuverture.minute);
      }
      lcd.print(" - ");
      lcd.print(heureFermeture.heure);
      lcd.print(":");
      if (heureFermeture.minute < 10) {
        lcd.print("0");
        lcd.print(heureFermeture.minute);
      }
      else {
        lcd.print(heureFermeture.minute);
      }
      lcd.print("           ");
    break;
    case 3: // Mode 3 : Luminosite
      lcd.print("Mode auto : ");
      lcd.print(etatPhotoCell);
      lcd.print("          ");
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
