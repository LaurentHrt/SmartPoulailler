const byte pinLedRouge=2, pinLedVerte=3, pinLedBleuMode1=4, pinLedVerteMode2=5, pinLedBlancheMode3=6;
const byte pinBuzzer=22;
const byte pinBoutonPorte=12, pinBoutonMode=11, pinPhotoCell=0;

byte mode; //1: Mode bouton, 2: Mode Luminosité, 3: Mode horaire

bool etatBouton, dernierEtatBouton, etatBoutonMode, dernierEtatBoutonMode;
bool porteOuverte;

int etatPhotoCell;


void setup() {

  Serial.begin(9600);

  mode=1;

  etatBouton=LOW;
  dernierEtatBouton=HIGH;
  etatBoutonMode=HIGH;
  dernierEtatBoutonMode=HIGH;
  porteOuverte=LOW;

  pinMode(pinLedRouge,OUTPUT);
  pinMode(pinLedVerte,OUTPUT);
  pinMode(pinLedBleuMode1,OUTPUT);
  pinMode(pinLedVerteMode2,OUTPUT);
  pinMode(pinLedBlancheMode3,OUTPUT);

  pinMode(pinBuzzer,OUTPUT);
  digitalWrite(pinBuzzer,HIGH);

  pinMode(pinBoutonPorte,INPUT_PULLUP);
  pinMode(pinBoutonMode,INPUT_PULLUP);

}

void loop() {

  // ************* Gestion du mode *******************
  etatBoutonMode=digitalRead(pinBoutonMode);
  if(etatBoutonMode!=dernierEtatBoutonMode && etatBoutonMode==LOW) {

    buzz(100);

    mode++;
    if (mode > 3)
      mode=1;
  }

  switch (mode)
  {
    case 1: // Mode 1 : Bouton
      digitalWrite(pinLedBleuMode1,LOW);
      digitalWrite(pinLedVerteMode2,HIGH);
      digitalWrite(pinLedBlancheMode3,HIGH);
      modeBouton();
    break;
    case 2: // Mode 2 : Luminosite
      digitalWrite(pinLedBleuMode1,HIGH);
      digitalWrite(pinLedVerteMode2,LOW);
      digitalWrite(pinLedBlancheMode3,HIGH);
      modeLuminosite();
    break;
    case 3: // Mode 3 : Horaire
      digitalWrite(pinLedBleuMode1,HIGH);
      digitalWrite(pinLedVerteMode2,HIGH);
      digitalWrite(pinLedBlancheMode3,LOW);
      modeHorraire();
    break;
    default:
      digitalWrite(pinLedBleuMode1,LOW);
      digitalWrite(pinLedVerteMode2,LOW);
      digitalWrite(pinLedBlancheMode3,LOW);
    break;
  }

  delay(200);

}

// Fonction : Mode 1 : Bouton
void modeBouton() {

  etatBouton=digitalRead(pinBoutonPorte);
  Serial.println(etatBouton);

  if(etatBouton!=dernierEtatBouton && etatBouton==LOW) {
    ouverturePorte(!porteOuverte);
  }

  dernierEtatBouton=etatBouton;

}

// Fonction : Mode 2 : Luminosite
void modeLuminosite() {

  etatPhotoCell=analogRead(pinPhotoCell);
  Serial.println(etatPhotoCell);

  if(etatPhotoCell>500)
    ouverturePorte(true);
  else
    ouverturePorte(false);

}

// Fonction : Mode 3 : Horaire
void modeHorraire() {

}

// Fonction déplacement de la porte (Juste des leds pour simuler pour l'instant)
// Param : Bool 1: action d'ouvrir la porteOuvert
//              2: action de fermer la porte
// Return : état de la porte
bool ouverturePorte(bool ouvrir) {
  if(!porteOuverte && ouvrir) {
    digitalWrite(pinLedRouge,HIGH);
    digitalWrite(pinLedVerte,LOW);
    buzz(100);
    porteOuverte=true;
    return porteOuverte;
  }
  else if (porteOuverte && !ouvrir) {
    digitalWrite(pinLedRouge,LOW);
    digitalWrite(pinLedVerte,HIGH);
    buzz(100);
    porteOuverte=false;
    return porteOuverte;
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
