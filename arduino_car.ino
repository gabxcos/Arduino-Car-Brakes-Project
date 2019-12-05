enum side { // enumerazione usata per distinguere se applicare ciascuna funzione ad uno solo
  LEFT,     // o ad entrambi i lati della macchina
  RIGHT,
  BOTH,
  NONE
};

// Per motivi di testing:
#define str(x)
#define xstr(x) str(x)

/*****************************************************************************/
/********************** PARAMETRI del MODELLO */

// ---- Sensore
long threshDist = 40;   // distanza minima dalla quale iniziare a valutare la presenza di un ostacolo
int bufferSize = 15;    // dimensione del contatore di buffer per ovviare a letture errate del sensore


// ---- Motori
float initVel = 0;        // velocità iniziale dei motori, in % (0-100)
float accel = 0.8;      // minima de/accelerazione
                        //  |-> N.B. costante moltiplicativa dell'AZIONE PROPORZIONALE
int warmup = 10;        // AZIONE COSTANTE nel caso in cui l'AZIONE PROPORZIONALE sia non-applicabile
                        // i.e. nessun ostacolo rilevato


                        


/*****************************************************************************/
/********************** VARIABILI, PIN e FUNZIONI di SUPPORTO */

/******* LED PIN */
int ledL = 12;
int ledR = 2;
int ledBwd = 3;


// ----
// Accendi le frecce per sterzare
void freccia(side s){
  if(s == LEFT){
    digitalWrite(ledL, HIGH);
    digitalWrite(ledR, LOW);
  }
  if(s == RIGHT){
    digitalWrite(ledR, HIGH);
    digitalWrite(ledL, LOW);
  }
  if(s == NONE){
    digitalWrite(ledL, LOW);
    digitalWrite(ledR, LOW);
  }
  if(s == BOTH){
    digitalWrite(ledL, HIGH);
    digitalWrite(ledR, HIGH);
  }
}

// Accendi le luci di stop
void stopLights(bool isOn){
  if(isOn) digitalWrite(ledBwd, HIGH);
  else digitalWrite(ledBwd, LOW);
  
}


/******* SENSORE AD ULTRASUONI */
int trigPinL = 30;     // Trigger pin del HC-SR04 sinistro
int echoPinL = 31;     // Echo pin del HC-SR04 sinistro
long durationL;        // Variabile per il calcolo della durata di ritorno del segnale
long distanceL;        // Variabili per il calcolo della distanza dell'ostacolo, data "duration"
int distanceBufferL = bufferSize; // Per ovviare a letture sbagliate intermedie
bool collidingL = true;

int trigPinR = 32;     // Trigger pin del HC-SR04 sinistro
int echoPinR = 33;     // Echo pin del HC-SR04 sinistro
long durationR;        // Variabile per il calcolo della durata di ritorno del segnale
long distanceR;        // Variabili per il calcolo della distanza dell'ostacolo, data "duration"
int distanceBufferR = bufferSize; // Per ovviare a letture sbagliate intermedie
bool collidingR = true;


// ----
// Sfrutta un intervallo minimo per valutare la distanza dell'ostacolo più vicino
void getWavesBack(side s){
  if(s == LEFT || s == BOTH){  
    digitalWrite(trigPinL, HIGH);     // send waves for 10 us
    delayMicroseconds(5);
    digitalWrite(trigPinL, LOW);
    
    durationL = pulseIn(echoPinL, HIGH); // receive reflected waves
    distanceL = durationL < 38000 ? (long)((float)durationL / 58.31) : (threshDist + warmup);   // convert to distance  
    delay(10);
  }
  if(s == RIGHT || s == BOTH){  
    digitalWrite(trigPinR, HIGH);     // send waves for 10 us
    delayMicroseconds(5);
    digitalWrite(trigPinR, LOW);
    
    durationR = pulseIn(echoPinR, HIGH); // receive reflected waves
    distanceR = durationR < 38000 ? (long)((float)durationR / 58.31) : (threshDist + warmup);   // convert to distance  
    delay(10);
  }
}

// Valuta se la distanza dell'ostacolo è sotto la soglia, come da parametro: se "è in collisione"
void isColliding(side s){
  if(s == LEFT || s == BOTH){
    if(distanceL > threshDist){
      if(distanceBufferL > 0){
        distanceBufferL--;
        collidingL = true;
      }
      else collidingL = false;
    }
    else{
      distanceBufferL = bufferSize;
      collidingL = true;
    }
  }
  if(s == RIGHT || s == BOTH){
    if(distanceR > threshDist){
      if(distanceBufferR > 0){
        distanceBufferR--;
        collidingR = true;
      }
      else collidingR = false;
    }
    else{
      distanceBufferR = bufferSize;
      collidingR = true;
    }
  }
}

side whereCollision(){
  isColliding(BOTH);
  if(collidingL && collidingR) return BOTH;
  else if(!(collidingL || collidingR)) return NONE;
  else if(collidingL && !collidingR) return LEFT;
  else if(collidingR && !collidingL) return RIGHT;
  else return NONE;
}

// Se l'ostacolo è "in collisione", valuta una distanza percentuale (0-100)
int mapDistance(side s){
  if(s == LEFT) return map(distanceL, 0, threshDist, 100, 0);
  else if(s == RIGHT) return map(distanceR, 0, threshDist, 100, 0);
  else return 100;
}


/******* MOTORI */
bool isStopped = false;    // Valuta se i motori sono fermi o meno
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
  isStopped = false;
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
  isStopped = false;
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
  if(s == BOTH) isStopped = true;
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
void decelerate(side s){ 
  stopLights(true);
  if(currVel <= 1) stopMotor(BOTH);
  else{
    int tempDistance = 0;
    if(s == BOTH){
      freccia(NONE);
      tempDistance = (int)(((float)mapDistance(LEFT)+(float)mapDistance(RIGHT))/2);
      forward(BOTH);
    }
    if(s == LEFT){
      freccia(LEFT);
      tempDistance = mapDistance(LEFT);
      stopMotor(RIGHT); // se l'ostacolo è a sinistra, vai a destra
      forward(LEFT);
    }
    if(s == RIGHT){
      freccia(RIGHT);
      tempDistance = mapDistance(RIGHT);
      stopMotor(LEFT); // se l'ostacolo è a sinistra, vai a destra
      forward(RIGHT);
    }
    
    if(tempDistance < 0) tempDistance = 0;
    if(tempDistance > 100) tempDistance = 100;
    
    currVel = max(0, (currVel -(tempDistance*accel)));
    if(currVel <= 1) stopMotor(BOTH);
  }
}

// Aggiorna la velocità corrente % in assenza di ostacolo, con accelerazione costante da parametro (warmup)
void accelerate(){
  stopLights(false);
  freccia(NONE);
  currVel = min(100, (currVel+warmup*accel));
  forward(BOTH);
}

// In caso di stop assoluto, torna indietro
void stepBack(){
  currVel = 50;  // resetta a velocità non-nulla
  updateSpeed(BOTH);
  
  stopLights(true);
  backward(BOTH);
  delay(500);
  stopLights(false);
  forward(BOTH);
}


// Decelera o accelera proattivamente sulla base della presenza o meno dell'ostacolo
void leggeControllo(side s){
  if(isStopped){
    stepBack();
  }else{
    if(s == NONE) accelerate();
    else decelerate(s);
  }
}

/*****************************************************************************/
/********************** SETUP */

void setup() {
  // LED
  pinMode(ledL, OUTPUT);
  pinMode(ledR, OUTPUT);
  pinMode(ledBwd, OUTPUT);
  // ULTRASUONI
  pinMode(trigPinL, OUTPUT);
  pinMode(echoPinL, INPUT);
  pinMode(trigPinR, OUTPUT);
  pinMode(echoPinR, INPUT);
  // MOTORI
  pinMode(enL, OUTPUT);
  pinMode(enR, OUTPUT);
  pinMode(backL, OUTPUT);
  pinMode(fwdL, OUTPUT);
  pinMode(backR, OUTPUT);
  pinMode(fwdR, OUTPUT);
  updateSpeed(BOTH); // Setta la velocità iniziale
  forward(BOTH); // Inizia muovendosi in avanti
  // ----------------------
  Serial.begin(9600);         // Inizializza la Seriale, per motivi di testing
  delay(1000);                // Aspetta 1 secondo prima di partire
}


                        


/*****************************************************************************/
/********************** LOOP */

void loop() {
  
  getWavesBack(BOTH); // Controlla la presenza di ostacoli

  side s = whereCollision();
  leggeControllo(s);
  
  updateSpeed(BOTH); // Imponi ai motori la nuova velocità richiesta


  /****** TESTING *******/
  /*
  Serial.println(distanceL);
  Serial.println(distanceR);
  Serial.println(s);
  Serial.println(isStopped);
  Serial.print("\n\n");

  Serial.print("\nAbs dist: ");
  Serial.println(distance);
  Serial.print("Perc dist: ");
  Serial.println(mapDistance());
  Serial.print("Perc vel: ");
  Serial.println(currVel);
  Serial.print("Abs vel: ");
  Serial.println(mapSpeed(currVel));*/
}
