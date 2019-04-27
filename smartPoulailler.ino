char pinLedRouge, pinLedVerte, pinLedBleuMode1, pinLedVerteMode2, pinLedBlancheMode3;
char pinBouton, pinBoutonMode;
char pinPhotoCell;
char mode; //1: Mode bouton, 2: Mode Luminosité, 3: Mode horaire

bool etatBouton, dernierEtatBouton, etatBoutonMode, dernierEtatBoutonMode;
bool porteOuverte;

int etatPhotoCell;


void setup() {

  Serial.begin(9600);

  mode=1;

  pinLedRouge=2;
  pinLedVerte=3;
  pinLedBleuMode1=4;
  pinLedVerteMode2=5;
  pinLedBlancheMode3=6;

  pinBouton=12;
  pinBoutonMode=11;
  pinPhotoCell=0;

  etatBouton=LOW;
  etatBoutonMode=LOW;
  dernierEtatBouton=HIGH;
  dernierEtatBoutonMode=HIGH;
  porteOuverte=LOW;

  pinMode(pinLedRouge,OUTPUT);
  pinMode(pinLedVerte,OUTPUT);
  pinMode(pinLedBleuMode1,OUTPUT);
  pinMode(pinLedVerteMode2,OUTPUT);
  pinMode(pinLedBlancheMode3,OUTPUT);

  pinMode(pinBouton,INPUT_PULLUP);
  pinMode(pinBoutonMode,INPUT_PULLUP);

}

void loop() {

  // ************* Gestion du mode *******************
  etatBoutonMode=digitalRead(pinBoutonMode);
  if(etatBoutonMode!=dernierEtatBoutonMode && etatBoutonMode==LOW) {
    mode++;
    if (mode > 3)
      mode=1;
  }
  // ************************************************

  // ************* Mode 1 : Bouton ******************
  if (mode==1) {

    digitalWrite(pinLedBleuMode1,LOW);
    digitalWrite(pinLedVerteMode2,HIGH);
    digitalWrite(pinLedBlancheMode3,HIGH);

    etatBouton=digitalRead(pinBouton);
    Serial.println(etatBouton);

    if(etatBouton!=dernierEtatBouton && etatBouton==LOW) {
      ouverturePorte(!porteOuverte);
    }

    dernierEtatBouton=etatBouton;
  }
  // ************************************************

  // ************* Mode 1 : Luminosite **************
  if (mode==2) {

    digitalWrite(pinLedBleuMode1,HIGH);
    digitalWrite(pinLedVerteMode2,LOW);
    digitalWrite(pinLedBlancheMode3,HIGH);

    etatPhotoCell=analogRead(pinPhotoCell);
    Serial.println(etatPhotoCell);

    if(etatPhotoCell>500)
      ouverturePorte(true);
    else
      ouverturePorte(false);
  }
  // ************************************************

  // ************* Mode 1 : Horaire ******************
  if (mode==3) {

    digitalWrite(pinLedBleuMode1,HIGH);
    digitalWrite(pinLedVerteMode2,HIGH);
    digitalWrite(pinLedBlancheMode3,LOW);

  }
  // ************************************************

  delay(200);

}

// Fonction déplacement de la porte (Juste des leds pour simuler pour l'instant)
// Param : Bool 1: action d'ouvrir la porteOuvert
//              2: action de fermer la porte
// Return : état de la porte
bool ouverturePorte(bool ouvrir) {
  if(!porteOuverte && ouvrir) {
    digitalWrite(pinLedRouge,HIGH);
    digitalWrite(pinLedVerte,LOW);
    porteOuverte=true;
    return porteOuverte;
  }
  else if (porteOuverte && !ouvrir) {
    digitalWrite(pinLedRouge,LOW);
    digitalWrite(pinLedVerte,HIGH);
    porteOuverte=false;
    return porteOuverte;
  }
  else
    return 0;
}
