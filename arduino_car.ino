#include <Servo.h> // Libreria per la gestione del servo motore, usato per
                   // far ruotare il sensore ad ultrasuoni

enum side { // enumerazione usata per distinguere se applicare ciascuna funzione ad uno solo
  LEFT,     // o ad entrambi i lati della macchina
  RIGHT,
  BOTH
};

/*****************************************************************************/
/********************** PARAMETRI del MODELLO */

// ---- Sensore
long threshDist = 70;   // distanza minima dalla quale iniziare a valutare la presenza di un ostacolo

// ---- Servo
int initAngle = 90;     // angolo iniziale del servo, 90° = centrale
int passo = 4;          // angolo di rotazione ad ogni loop, in gradi

// ---- Motori
int initVel = 0;        // velocità iniziale dei motori, in % (0-100)
float accel = 0.6;      // minima de/accelerazione
                        //  |-> N.B. costante moltiplicativa dell'AZIONE PROPORZIONALE
int warmup = 10;        // AZIONE COSTANTE nel caso in cui l'AZIONE PROPORZIONALE sia non-applicabile
                        // i.e. nessun ostacolo rilevato


                        


/*****************************************************************************/
/********************** VARIABILI, PIN e FUNZIONI di SUPPORTO */

/******* SENSORE AD ULTRASUONI */
int trigPin = 30;     // Trigger pin del HC-SR04
int echoPin = 31;     // Echo pin del HC-SR04
long duration;        // Variabile per il calcolo della durata di ritorno del segnale
long distance;        // Variabile per il calcolo della distanza dell'ostacolo, data "duration"

// ----
// Sfrutta un intervallo minimo per valutare la distanza dell'ostacolo più vicino
void getWavesBack(){
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);   
  digitalWrite(trigPin, HIGH);     // send waves for 10 us
  delayMicroseconds(10);
  duration = pulseIn(echoPin, HIGH); // receive reflected waves
  distance = duration / 58.2;   // convert to distance    
  delay(10);
}

// Valuta se la distanza dell'ostacolo è sotto la soglia, come da parametro: se "è in collisione"
bool isColliding(){
  if(distance > threshDist) return false;
  else return true;
}

// Se l'ostacolo è "in collisione", valuta una distanza percentuale (0-100)
int mapDistance(){
  return map(distance, 0, threshDist, 100, 0);
}




/******* MOTORI */
int currVel = initVel;  // Variabile % (0-100) per la velocità dei motori

                        // N.B. poiché la direzione di svolta è opposta al lato del motore acceso,
                        // le direzioni espresse nelle variabili sono volutamente invertite di lato,
                        // per semplificare la logica delle funzioni di supporto
// - LATO SINISTRO
int enL = 10;          // ENABLE B, controllo velocità del motore sx tramite PWM
int fwdL = 9;         // connettore di marcia normale SISTRO   
int backL = 8;        // connettore di retromarcia SINISTRO
// - LATO DESTRO
int fwdR = 7;         // connettore di marcia normale DESTRO
int backR = 6;        // connettore di retromarcia DESTRO
int enR = 5;         // ENABLE A, controllo velocità del motore dx tramite PWM

// ----
// Imposta uno/entrambi i motori per il movimento in avanti
void forward(side s){
  if(s == LEFT || s == BOTH){
    digitalWrite(fwdL, HIGH);                                
    digitalWrite(backL, LOW);
  }
  if(s == RIGHT || s == BOTH){
    digitalWrite(fwdR, HIGH);
    digitalWrite(backR, LOW);
  }
}

// Imposta uno/entrambi i motori per il movimento in retromarcia
void backward(side s){
  if(s == LEFT || s == BOTH){
    digitalWrite(fwdL, LOW);                                
    digitalWrite(backL, HIGH);
  }
  if(s == RIGHT || s == BOTH){
    digitalWrite(fwdR, LOW);
    digitalWrite(backR, HIGH);
  }
}

// Ferma uno/entrambi i motori
void stopMotor(side s){
  if(s == LEFT || s == BOTH){
    digitalWrite(fwdL, LOW);                                
    digitalWrite(backL, LOW);
  }
  if(s == RIGHT || s == BOTH){
    digitalWrite(fwdR, LOW);
    digitalWrite(backR, LOW);
  }
}

// Ottiene la velocità in PWM, data quella percentuale desiderata
int mapSpeed(int percSpeed){
  return map(percSpeed, 0, 100, 60, 100); // 75-255 intervallo sperimentale di funzionamento
}

// Applica la velocità corrente % a uno/entrambi i motori
void updateSpeed(side s){
  if(s == LEFT || s == BOTH){
    analogWrite(enL, mapSpeed(currVel));      
  }
  if(s == RIGHT || s == BOTH){
    analogWrite(enR, mapSpeed(currVel));
  }
}

// Aggiorna la velocità corrente % in base alla prossimità dall'ostacolo
void decelerate(){ 
  currVel = max(0, (int)(currVel-(mapDistance()*accel)));
  if(currVel <= 1) stopMotor(BOTH);
  else forward(BOTH);
}

// Aggiorna la velocità corrente % in assenza di ostacolo, con accelerazione costante da parametro (warmup)
void accelerate(){
  currVel = min(100, (int)(currVel+warmup*accel));
  forward(BOTH);
}


/******* SERVO */
Servo myservo;        // oggetto Servo per controllare il servomotore tramite "Servo.h"
int angle = initAngle;// angolo del servo, 0° = sx, 180° = dx
int dir = -1;         // variabile per la direzione di rotazione del servo, -1 = sx, +1 = dx

// ----
// Aggiorna ad ogni loop l'angolo, muovendosi di "passo" gradi nella direzione corrente, eventualmente invertendola
void updateAngle(){
  if(angle <= 0 || angle >= 180) dir *= -1;
  angle += dir*passo;
}


                        


/*****************************************************************************/
/********************** SETUP */

void setup() {
  // ULTRASUONI
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  // MOTORI
  pinMode(enL, OUTPUT);
  pinMode(enR, OUTPUT);
  pinMode(backL, OUTPUT);
  pinMode(fwdL, OUTPUT);
  pinMode(backR, OUTPUT);
  pinMode(fwdR, OUTPUT);
  updateSpeed(BOTH); // Setta la velocità iniziale
  // SERVO
  myservo.attach(11);
  myservo.write(angle);       // Posiziona il servo all'angolo iniziale
  // ----------------------
  Serial.begin(9600);         // Inizializza la Seriale, per motivi di testing
  delay(1000);                // Aspetta 1 secondo prima di partire
  forward(BOTH);              // Inizia muovendosi in avanti
}


                        


/*****************************************************************************/
/********************** LOOP */

void loop() {
  
  getWavesBack(); // Controlla la presenza di ostacoli

  if (!isColliding())   // CASO 1: NON SONO PRESENTI OSTACOLI         
  {
    accelerate(); // Aumenta la velocità, se non ha raggiunto il massimo (100%)
    //updateAngle(); // Muovi il servo
    //myservo.write(angle);                                                   
  }

  else                  // CASO 2: OSTACOLO IN ROTTA DI COLLISIONE
  {
    decelerate(); // Diminuisci la velocità, sulla base della distanza dall'oggetto
  }
  
  updateSpeed(BOTH); // Imponi ai motori la nuova velocità richiesta

  /****** TESTING *******/
  Serial.print("\nAbs dist: ");
  Serial.println(distance);
  Serial.print("Perc dist: ");
  Serial.println(mapDistance());
  Serial.print("Perc vel: ");
  Serial.println(currVel);
  Serial.print("Abs vel: ");
  Serial.println(mapSpeed(currVel));
}
