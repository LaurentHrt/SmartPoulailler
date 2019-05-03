// Declaration des includes
#include <LiquidCrystal.h>  // LCD
#include <Wire.h>           // RTC
#include <RTClib.h>         // RTC
#include <SimpleDHT.h>      // Temp + Humidity
#include <math.h>


// Declaration des constantes
const byte pinLedRouge=2, pinLedVerte=3;
const byte pinBuzzer=22;
const byte pinBoutonPorte=12, pinBoutonMode=11;
const byte pinPhotoCell=0;
const byte pinDHT=9;
const int seuilLuminosite=500;                                          // TODO: A definir
const DateTime heureOuverture = DateTime(2019, 04, 30, 7, 00, 00);      // TODO: A definir
const DateTime heureFermeture = DateTime(2019, 04, 30, 20, 00, 00);     // TODO: A definir
const long tempoLuminosite = 5000;                                      // TODO: A definir
const byte heureMatinMin = 5;
const byte heureMatinMax = 10;
const byte heureSoirMin = 16;
const byte heureSoirMax = 23;
const long tempoTemperature = 60000;

// Declaration des variables globales
byte mode; // 1: Mode bouton, 2: Mode Luminosité, 3: Mode horaire
bool etatBouton, dernierEtatBouton, etatBoutonMode, dernierEtatBoutonMode;
bool etatPorte;
int etatPhotoCell;
DateTime now;
unsigned long currentMillis;
unsigned long previousMillisLuminosite, previousMillisTemperature;
SimpleDHT11 dht11(pinDHT);
byte temperature;
byte humidity;

// Declaration du LCD (numero de pin)
LiquidCrystal lcd(38, 39, 40, 41, 42, 43);

// Declaration de l'horloge
RTC_DS3231 rtc;


  double lever, meridien, coucher;
  DateTime test;

void setup() {

  // Demarrage du port serie
  Serial.begin(9600);

  // Demarrage de l'horloge
  rtc.begin();
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

  // Premiere aquisition de la temperature
  dht11.read(&temperature, &humidity, NULL);

  buzz(100);


  test=rtc.now();
  calculerEphemeride(test.day(), test.month(), test.year(), 47.730420, -7.148910, &lever, &meridien, &coucher);



}

void loop() {


    Serial.print("Lever : ");
    afficherHeure(lever);
    Serial.println("");
    Serial.print("Coucher : ");
    afficherHeure(coucher);
    Serial.println("");



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

  now = rtc.now();

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

  // Aquisition de l'heure actuelle
  now = rtc.now();

  // Mise a jour du LCD
  lcd.setCursor(0, 0);
  lcd.print("Mode H : ");
  lcd.print(heureOuverture.hour(), DEC);
  lcd.print("H-");
  lcd.print(heureFermeture.hour(), DEC);
  lcd.print("H       ");

  // Action sur la porte
  if(now.hour()==heureOuverture.hour() && now.minute()==heureOuverture.minute())
    ouverturePorte(true);
  else if (now.hour()==heureFermeture.hour() && now.minute()==heureFermeture.minute())
    ouverturePorte(false);

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






void calculerCentreEtVariation(double longitude_ouest, double sinlat, double coslat, double d, double *centre, double *variation)
{
  //constantes précalculées par le compilateur
  const double M_2PI = 2.0 * M_PI;
  const double degres = 180.0 / M_PI;
  const double radians = M_PI / 180.0;
  const double radians2 = M_PI / 90.0;
  const double m0 = 357.5291;
  const double m1 = 0.98560028;
  const double l0 = 280.4665;
  const double l1 = 0.98564736;
  const double c0 = 0.01671;
  const double c1 = degres * (2.0*c0 - c0*c0*c0/4.0);
  const double c2 = degres * c0*c0 * 5.0 / 4.0;
  const double c3 = degres * c0*c0*c0 * 13.0 / 12.0;
  const double r1 = 0.207447644182976; // = tan(23.43929 / 180.0 * M_PI / 2.0)
  const double r2 = r1*r1;
  const double d0 = 0.397777138139599; // = sin(23.43929 / 180.0 * M_PI)
  const double o0 = -0.0106463073113138; // = sin(-36.6 / 60.0 * M_PI / 180.0)

  double M,C,L,R,dec,omega,x;

  //deux ou trois petites formules de calcul
  M = radians * fmod(m0 + m1 * d, 360.0);
  C = c1*sin(M) + c2*sin(2.0*M) + c3*sin(3.0*M);
  L = fmod(l0 + l1 * d + C, 360.0);
  x = radians2 * L;
  R = -degres * atan((r2*sin(x))/(1+r2*cos(x)));
  *centre = (C + R + longitude_ouest)/360.0;

  dec = asin(d0*sin(radians*L));
  omega = (o0 - sin(dec)*sinlat)/(cos(dec)*coslat);
  if ((omega > -1.0) && (omega < 1.0))
    *variation = acos(omega) / M_2PI;
  else
    *variation = 0.0;
}

void calculerEphemeride(int jour, int mois, int annee, double longitude_ouest, double latitude_nord, double *lever, double *meridien, double *coucher)
{
  int nbjours;
  const double radians = M_PI / 180.0;
  double d, x, sinlat, coslat;

  //calcul nb jours écoulés depuis le 01/01/2000
  if (annee > 2000) annee -= 2000;
  nbjours = (annee*365) + ((annee+3)>>2) + jour - 1;
  switch (mois)
  {
    case  2 : nbjours +=  31; break;
    case  3 : nbjours +=  59; break;
    case  4 : nbjours +=  90; break;
    case  5 : nbjours += 120; break;
    case  6 : nbjours += 151; break;
    case  7 : nbjours += 181; break;
    case  8 : nbjours += 212; break;
    case  9 : nbjours += 243; break;
    case 10 : nbjours += 273; break;
    case 11 : nbjours += 304; break;
    case 12 : nbjours += 334; break;
  }
  if ((mois > 2) && (annee&3 == 0)) nbjours++;
  d = nbjours;

  //calcul initial meridien & lever & coucher
  x = radians * latitude_nord;
  sinlat = sin(x);
  coslat = cos(x);
  calculerCentreEtVariation(longitude_ouest, sinlat, coslat, d + longitude_ouest/360.0, meridien, &x);
  *lever = *meridien - x;
  *coucher = *meridien + x;

  //seconde itération pour une meilleure précision de calcul du lever
  calculerCentreEtVariation(longitude_ouest, sinlat, coslat, d + *lever, lever, &x);
  *lever = *lever - x;

  //seconde itération pour une meilleure précision de calcul du coucher
  calculerCentreEtVariation(longitude_ouest, sinlat, coslat, d + *coucher, coucher, &x);
  *coucher = *coucher + x;
}

void afficherHeure(double d)
{
  int h,m,s;
  char texte[20];

  d = d + 0.5;

  if (d < 0.0)
  {
    d = d + 1.0;
  }
  else
  {
    if (d > 1.0)
    {
      d = d - 1.0;
    }
    else
    {

    }
  }

  h = d * 24.0;
  d = d - (double) h / 24.0;
  m = d * 1440.0;
  d = d - (double) m / 1440.0;
  s = d * 86400.0 + 0.5;

  sprintf(texte,"%02d:%02d:%02d",h,m,s);
  Serial.print(texte);

  // Serial.println(texte,"%02d:%02d:%02d",h,m,s);
  // Serial.print(h);
  // Serial.print(":");
  // Serial.print(m);
  // Serial.print(":");
  // Serial.print(s);
}
