char pinLedRouge;
char pinLedVerte;
char pinBouton;
char pinPhotoCell;

bool etatBouton, dernierEtatBouton;
bool porteOuverte;

int etatPhotoCell;

void setup() {

  Serial.begin(9600);

  pinLedRouge=2;
  pinLedVerte=3;
  pinBouton=12;
  pinPhotoCell=0;

  etatBouton=LOW;
  dernierEtatBouton=HIGH;
  porteOuverte=LOW;

  pinMode(pinLedRouge,OUTPUT);
  pinMode(pinLedVerte,OUTPUT);
  pinMode(pinBouton,INPUT_PULLUP);

}

void loop() {

  etatBouton=digitalRead(pinBouton);
  etatPhotoCell=analogRead(pinPhotoCell);
  Serial.println(etatPhotoCell);
  //Serial.println(etatBouton);

  if(etatBouton!=dernierEtatBouton && etatBouton==LOW) {
    if(porteOuverte) {
      digitalWrite(pinLedRouge,HIGH);
      digitalWrite(pinLedVerte,LOW);
    }
    else {
      digitalWrite(pinLedRouge,LOW);
      digitalWrite(pinLedVerte,HIGH);
    }
    porteOuverte=!porteOuverte;
  }

  dernierEtatBouton=etatBouton;

  if(etatPhotoCell>500) {
    digitalWrite(pinLedRouge,HIGH);
    digitalWrite(pinLedVerte,LOW);
  }
  else {
    digitalWrite(pinLedRouge,LOW);
    digitalWrite(pinLedVerte,HIGH);
  }

  delay(200);

}
