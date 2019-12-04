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
long threshDist = 50;   // distanza minima dalla quale iniziare a valutare la presenza di un ostacolo
int bufferSize = 15;    // dimensione del contatore di buffer per ovviare a letture errate del sensore

// ---- Servo
int initAngle = 90;     // angolo iniziale del servo, 90° = centrale
int passo = 2;          // angolo di rotazione ad ogni loop, in gradi

// ---- Motori
float initVel = 100;        // velocità iniziale dei motori, in % (0-100)
float accel = 0.8;      // minima de/accelerazione
                        //  |-> N.B. costante moltiplicativa dell'AZIONE PROPORZIONALE
int warmup = 10;        // AZIONE COSTANTE nel caso in cui l'AZIONE PROPORZIONALE sia non-applicabile
                        // i.e. nessun ostacolo rilevato


                        


/*****************************************************************************/
/********************** VARIABILI, PIN e FUNZIONI di SUPPORTO */

/******* SENSORE AD ULTRASUONI */
int trigPin = 30;     // Trigger pin del HC-SR04
int echoPin = 31;     // Echo pin del HC-SR04
long duration;        // Variabile per il calcolo della durata di ritorno del segnale
long distance;        // Variabili per il calcolo della distanza dell'ostacolo, data "duration"
int distanceBuffer = bufferSize; // Per ovviare a letture sbagliate intermedie

// ----
// Sfrutta un intervallo minimo per valutare la distanza dell'ostacolo più vicino
void getWavesBack(){
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);   
  digitalWrite(trigPin, HIGH);     // send waves for 10 us
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  duration = pulseIn(echoPin, HIGH); // receive reflected waves
  distance = duration < 38000 ? (long)((float)duration / 58.31) : 100;   // convert to distance  
  delay(30);
}

// Valuta se la distanza dell'ostacolo è sotto la soglia, come da parametro: se "è in collisione"
bool isColliding(){
  if(distance > threshDist){
    if(distanceBuffer > 0){
      distanceBuffer--;
      return true;
    }
    else return false;
  }
  else{
    distanceBuffer = bufferSize;
    return true;
  }
}

// Se l'ostacolo è "in collisione", valuta una distanza percentuale (0-100)
int mapDistance(){
  return map(distance, 0, threshDist, 100, 0);
}


/******* SERVO */
Servo myservo;        // oggetto Servo per controllare il servomotore tramite "Servo.h"
int angle = initAngle;// angolo del servo, 0° = sx, 180° = dx
int dir = -1;         // variabile per la direzione di rotazione del servo, -1 = sx, +1 = dx

// ----
// Aggiorna ad ogni loop l'angolo, muovendosi di "passo" gradi nella direzione corrente, eventualmente invertendola
void updateAngle(){
  if(angle <= 45 || angle >= 135) dir *= -1;
  angle += dir*passo;
}


/******* MOTORI */
float currVel = initVel;  // Variabile % (0-100) per la velocità dei motori

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
  return map(percSpeed, 0, 100, 60, 150); // 75-255 intervallo sperimentale di funzionamento
}

// Applica la velocità corrente % a uno/entrambi i motori
void updateSpeed(side s){
  if(s == LEFT || s == BOTH){
    analogWrite(enL, mapSpeed((int)currVel));      
  }
  if(s == RIGHT || s == BOTH){
    analogWrite(enR, mapSpeed((int)currVel));
  }
}

// Aggiorna la velocità corrente % in base alla prossimità dall'ostacolo
void decelerate(){ 
  int tempDistance = mapDistance();
  if(tempDistance < 0) tempDistance = 0;
  if(tempDistance > 100) tempDistance = 100;
  
  currVel = max(0, (currVel -(tempDistance*accel)));
  if(currVel <= 1) stopMotor(BOTH);
  else{
    if(angle < 90){
      stopMotor(LEFT); // se l'ostacolo è a sinistra, vai a destra
      forward(RIGHT);
    }
    if(angle < 90){
      stopMotor(RIGHT); // se l'ostacolo è a destra, vai a sinistra
      forward(LEFT);
    }
    else forward(BOTH);
  }
}

// Aggiorna la velocità corrente % in assenza di ostacolo, con accelerazione costante da parametro (warmup)
void accelerate(){
  currVel = min(100, (currVel+warmup*accel));
  forward(BOTH);
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
}


                        


/*****************************************************************************/
/********************** LOOP */

void loop() {
  
  getWavesBack(); // Controlla la presenza di ostacoli

  if (!isColliding())   // CASO 1: NON SONO PRESENTI OSTACOLI         
  {
    accelerate(); // Aumenta la velocità, se non ha raggiunto il massimo (100%)
    updateAngle(); // Muovi il servo
    myservo.write(angle);                                                   
  }

  else                  // CASO 2: OSTACOLO IN ROTTA DI COLLISIONE
  {
    decelerate(); // Diminuisci la velocità, sulla base della distanza dall'oggetto
  }
  
  updateSpeed(BOTH); // Imponi ai motori la nuova velocità richiesta



  /****** TESTING *******/
  /*Serial.print("\nAbs dist: ");
  Serial.println(distance);
  Serial.print("Perc dist: ");
  Serial.println(mapDistance());
  Serial.print("Perc vel: ");
  Serial.println(currVel);
  Serial.print("Abs vel: ");
  Serial.println(mapSpeed(currVel));*/
}
