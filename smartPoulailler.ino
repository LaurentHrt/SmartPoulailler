char pinLedRouge;
char pinLedVerte;
char pinBouton;
bool etatBouton, dernierEtatBouton;
bool porteOuverte;

void setup() {

  Serial.begin(9600);

  pinLedRouge=2;
  pinLedVerte=3;
  pinBouton=12;

  etatBouton=LOW;
  dernierEtatBouton=HIGH;
  porteOuverte=LOW;

  pinMode(pinLedRouge,OUTPUT);
  pinMode(pinLedVerte,OUTPUT);
  pinMode(pinBouton,INPUT_PULLUP);

}

void loop() {

  etatBouton=digitalRead(pinBouton);
  Serial.println(etatBouton);

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

  delay(200);

}
