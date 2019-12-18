# K.I.T.T. 2.0

Progetto per Tecnologia dei Sistemi di Controllo, A.A. 2019/20

A cura di:
Andrea Arciprete, Bartolomeo Caruso, Daniela Chiavetta, Gabriele Costanzo, Giuseppe Fallica



1. **INTRODUZIONE:**

Il nostro progetto per il corso di &quot;Tecnologia dei Sistemi di Controllo&quot;, tenuto dalla prof.ssa Ing. L.V. Gambuzza, riguarda l&#39;implementazione dei concetti appresi su controllori digitali (on/off e PID), sensori e attuatori al fine di implementare un prototipo che simuli un sistema di controllo reale.

La nostra attenzione come gruppo di lavoro si è concentrata sui sistemi presenti all&#39;interno delle automobili, e in particolare il controllo sulla frenata e l&#39;individuazione degli ostacoli attraverso sensori di prossimità.

Il nostro prototipo prende il nome di **K.I.T.T. 2.0** , in onore del veicolo protagonista della serie cult degli anni &#39;80 _&quot;Supercar&quot; (Knight Rider, orig.)_: utilizza due motori DC per il movimento della macchina, e una coppia di moduli sensori ad ultrasuoni per l&#39;individuazione dell&#39;ostacolo. In aggiunta, si è scelto di decorare esteticamente il prototipo con un set di tre led, due gialli frontali per rappresentare le frecce di svolta, uno posteriore rosso di retromarcia e frenata.

Per una dimostrazione visiva del funzionamento del nostro prototipo si rimanda al video in allegato a questa documentazione. Il suddetto documento, invece, si soffermerà sull&#39;implementazione software del circuito e sui dettagli più tecnici dell&#39;hardware che lo utilizza: lo si può considerare come un README del codice presente nel file _&quot;arduino\_car.ino&quot;_.

Si consiglia ugualmente una lettura preventiva, per intero, del codice, che è adeguatamente commentato.



2. **CONSIDERAZIONI INIZIALI:**

Il funzionamento del prototipo non è perfetto, motivo per cui l&#39;implementazione software tiene conto anche di un comportamento imprevisto da correggere, al di là di quello che valuta regolarmente come errore, in termini di controllore. Si prendano come esempi immediati:

- errori nella forma degli scomparti contenenti i motori, telaio su cui sono assemblati, imperfezioni nella gomma delle ruote, sono tutti fattori che impediscono una marcia perfettamente rettilinea della macchina, per quanto il codice non abbia modo di accorgersi di questa tipologia di errore;

- dei ritardi nello spegnimento o nell&#39;accensione dei motori – rispetto al momento in cui si chiede tramite codice di generare il relativo segnale – possono ulteriormente provocare movimenti imprevisti.

Per ovviare in maniera comoda a questa tipologia di problemi, è tornato molto utile distinguere variabili e costanti a run-time, ovvero usate dal codice per gestire l&#39;individuazione e la correzione dello stato, da quelle che invece possiamo considerare **PARAMETRI del MODELLO** del nostro prototipo. A titolo d&#39;esempio, consideriamo parametri del modello la distanza minima di individuazione dell&#39;ostacolo e l&#39;accelerazione minima della macchina.

Il codice è commentato passo-passo per agevolarne la lettura, inoltre ogni funzione o variabile è separata dalle altre per funzionalità (led, sensori, motori, etc.), permettendo quindi di seguire singoli blocchi di codice mentre si legge il resto del documento.

3. **ALIMENTAZIONE e STRUMENTAZIONE AGGIUNTIVA**

Una delle sfide principali della costruzione del nostro prototipo è stata quella dell&#39;alimentazione del circuito: i due motori in DC non possono essere alimentati direttamente dall&#39;Arduino Mega utilizzato, poiché non potrebbe fornire una corrente sufficiente per avere un movimento apprezzabile. Per questo, abbiamo utilizzato un modulo driver L298N: prendendo in ingresso 12 V da un apposito vano batterie, può controllare ciascuno dei due motori tramite tre segnali di controllo, gestiti dall&#39;Arduino: un enable PWM, che permette di fornire al motore qualunque tensione fra quelle minima e massima gestite dal driver, in maniera proporzionale; due input digitali, tali da decidere la direzione di movimento del motore (in una direzione se il primo input è HIGH e l&#39;altro low, in direzione opposta invertendo i due livelli, e nessun movimento se entrambi gli input sono a HIGH o entrambi a LOW).

Risolto il problema dell&#39;alimentazione dei motori, l&#39;Arduino Mega deve comunque gestire tre led con le rispettive resistenze in serie e due sensori a ultrasuoni: per non rubare troppa corrente al vano batterie (pena non alimentare correttamente i motori), si è optato per alimentare separatamente il microcontrollore tramite una power bank, collegata all&#39;ingresso USB.

4. **SENSORE a ULTRASUONI**

Come sensori si è scelto di usare due coppie di sensori ad ultrasuoni HC-SR04, direzionati in modo tale da rilevare oggetti leggermente spostati più da un lato che da un altro. Ciascuno di questi due sensori funziona generando un&#39;onda ad ultrasuoni, attendendo un delay (5 us) tale da permettere il cammino dell&#39;onda contro un ostacolo, e rilevando l&#39;onda riflessa usando la funzione **pulseIn** , che ci fornisce il ritardo di propagazione e ritorno dell&#39;onda. Da questo ritardo, con una giusta approssimazione della velocità del suono (a temperatura ambiente), otteniamo la distanza del sensore dal primo ostacolo utile, in cm.

Il parametro del modello **threshDist** (distanza di threshold) determina la distanza in cm sotto la quale si inizia a rilevare la presenza dell&#39;ostacolo, e conseguentemente si inizia a decelerare. Inoltre, la funzione **mapDistance** ci permette di ottenere un valore percentuale della distanza, dove threshDist equivale allo 0% e si sale man mano che diminuisce la distanza: questo tornerà utile per la legge di controllo.

Il codice prevede anche delle istruzioni di fallback, per i due principali casi d&#39;errore del sensore: un ostacolo troppo vicino (meno di 4 cm) o troppo distante (nessuna rilevazione dell&#39;onda di ritorno).

Poiché ugualmente è possibile che il sensore ad ultrasuoni rilevi in maniera sbagliata la distanza dall&#39;ostacolo, è stato previsto un meccanismo di **buffer** : alla prima rilevazione dell&#39;ostacolo, viene inizializzato un contatore, che viene poi decrementato ad ogni successiva iterazione in cui non si rileva nessun ostacolo di fronte alla macchina. La probabilità che così tante rilevazioni consecutive siano tutte errate, infatti, è apprezzabilmente bassa; se non si sfruttasse questo meccanismo, una singola rilevazione errata porterebbe ad un&#39;accelerazione della macchina, e conseguentemente all&#39;urto con l&#39;ostacolo.

Delle funzioni di utility, **isColliding** e **whereCollision** , ci permettono di comprendere la posizione dell&#39;ostacolo: qualora entrambi i sensori valutino la presenza dell&#39;ostacolo, è stato definito un parametro del modello, **diffDist** , che serve a decretare se le distanze rilevate per l&#39;ostacolo dai due sensori sono sufficientemente differenti (almeno il 50% della distanza di threshold, con il valore corrente) da poter decretare che l&#39;ostacolo è più imminente da un lato, che dall&#39;altro. Ciò succede, ovviamente, anche se l&#39;ostacolo è rilevato da uno solo dei due sensori.

Diversamente, si valuta che l&#39;ostacolo sia perfettamente frontale alla macchina.

5. **ATTUATORE: MOTORI DC**

Il funzionamento a livello di segnali di controllo e di alimentazione dei due motori è stato spiegato al punto 3).

La macchina usa solo due motori ed un carrellino con una ruota libera. Non potendo modificare la direzione delle ruote, si sfrutta il seguente principio per sterzare: se si decide di girare a sinistra, è sufficiente fermare la ruota sinistra e mantenere in movimento quella destra, e viceversa per girare a destra.



Decisi due valori, minimo e massimo, per i segnali PWM da inviare come enable dei motori, a cui quindi fanno riferimento una tensione minima e massima, si mappano linearmente questi due valori su una scala percentuale tramite la funzione **mapSpeed** , così da lavorare su velocità percentuali comprese fra 0 e 100. Si noti che la tensione legata al valore di velocità 0 non coincide con un&#39;effettiva velocità nulla dei motori, per cui all&#39;azzeramento della velocità va accompagnato un effettivo spegnimento dei motori ponendo entrambi gli ingressi al livello LOW.

Un intero set di funzioni è stato scritto per porre i motori in forward, backward o stop (direzione), o per aggiornare la velocità sulla base di un&#39;accelerazione costante che è parametro del modello, usata sia per accelerare che per decelerare. Tutte le funzioni possono essere chiamate per uno solo dei due lati o per entrambi contemporaneamente.

La legge di controllo agisce sui motori decidendo come aggiornare la velocità assoluta, usata per entrambi i motori, e se eventualmente fermarne uno o entrambi. Infine, nel caso in cui si arrivi effettivamente a fermarsi per evitare l&#39;ostacolo, è prevista una breve retromarcia in direzione tale da evitare poi l&#39;ostacolo se si riprende la normale marcia in avanti.

6. **LEGGE di CONTROLLO**

La funzione **leggeControllo** si occupa sia di comandare i movimenti della macchina, in base all&#39;ultimo stato trovato, sia di applicare l&#39;effettiva legge di controllo reale laddove necessaria.

Nello specifico:

- nel caso in cui non vi sia alcun ostacolo di fronte alla macchina, chiama la funzione accelerate, la cui legge è – banalmente – di aumentare la velocità in base alla accelerazione di base **accel** (presa dai parametri del modello), moltiplicata per un fattore costante **warmup** , ad ogni ciclo del loop;

- nel caso in cui vi sia un ostacolo di fronte alla macchina, chiama la funzione **decelerate** , la cui legge di controllo è più interessante e necessita di essere trattata più in dettaglio; inoltre, mentre la velocità viene gradualmente diminuita sempre con la medesima legge, in base che l&#39;ostacolo sia laterale anziché frontale la funzione decide di fermare opportunamente una delle due ruote per integrare l&#39;azione di rallentamento della macchina con una di sterzata;

- nel caso in cui la macchina, rallentando man mano, sia arrivata a fermarsi del tutto, si decide arbitrariamente di fare retromarcia per un piccolo intervallo di tempo (opportunamente, in direzione opposta rispetto all&#39;ostacolo) chiamando la funzione **stepBack** : infatti, se la macchina dovesse continuare a percepire la presenza dell&#39;ostacolo non proverebbe mai ad accelerare nuovamente.

La legge che permette alla macchina di decelerare in presenza dell&#39;ostacolo prevede un contributo di tipo PROPORZIONALE, rispetto alla distanza percentuale (si veda **mapDistance** ) percepita come &quot;errore da correggere&quot;, ovvero si vuole che tale distanza si mantenga quanto più possibile alta.

Sia **currVel** la variabile che contiene la velocità corrente da applicare ai motori (espressa in percentuale, da 0 a 100), allora la velocità verrà diminuita secondo la legge:

`
currVel (K+1) = currVel (K) – tempDistance \* accel
`

Dove si è già detto che _accel_ è un parametro del modello, ed è la minima decelerazione effettuabile dalla macchina, mentre _tempDistance_ è una variabile temporanea dove si valuta il fattore d&#39;impatto contro l&#39;ostacolo in percentuale, dove una distanza pari a quella di threshold vale per lo 0%, e una distanza nulla dall&#39;ostacolo vale per il 100%.

Raggiunta una velocità currVel pari allo 0%, si impone lo spegnimento dei motori oltre a scalare linearmente il segnale enable PWM.

Intuitivamente, l&#39;azione &quot;decelerativa&quot; sarà quindi tanto più forte quanto più l&#39;ostacolo è vicino: il comportamento è chiaramente ispirato al feedback visivo del guidatore, che rallenta man mano che si avvicina ad un ostacolo, senza frenare all&#39;improvviso se non è necessario; e invece frena con maggior forza se l&#39;ostacolo è molto ravvicinato e si cammina a velocità sostenuta.

7. **SETUP, LOOP e DETTAGLI CONCLUSIVI**

Il **setup** si occupa di settare tutti i pin alle rispettive modalità di INPUT o OUTPUT, inizializzare i motori alla PWM corrispondente alla velocità nulla e imporre il movimento in avanti.

Il **loop** del codice prevede una prima chiamata alla funzione che attiva i sensori ad ultrasuoni e aggiorna quindi le relative distanze sx/dx dagli ostacoli. Una successiva chiamata alla funzione **whereCollision** ci permette anche di conoscere il &quot;lato&quot; in cui è (o non è) presente l&#39;ostacolo.

Passando il suddetto &quot;lato&quot; alla legge di controllo, la velocità e lo stato dei motori vengono aggiornati coerentemente secondo quanto sopra. Infine, una chiamata alla funzione **updateSpeed** impone la nuova velocità ottenuta tramite la legge ad entrambi i motori.

Si noti che non ci si è soffermati sul funzionamento dei led, ma è facile intuire dalla lettura del codice che il loro scopo è quello di mostrare il lato verso cui avviene la sterzata per evitare l&#39;ostacolo, o mostrare lo stato di stop o retromarcia.

8. **CONSIDERAZIONI FUTURE**

Se si dovesse decidere in futuro di ampliare il progetto, vi sono svariate idee da poter implementare: innanzitutto, la presenza di sensori anche nel retro permetterebbe un controllo quasi a 360° dello spazio circostante, e una più ampia libertà di movimento; si potrebbe poi optare di implementare il tutto per un sistema a quattro ruote motrici, anziché due, per garantire una maggiore stabilità e un movimento più &quot;naturale&quot;; ancora, un&#39;altra idea balenata durante la progettazione riguarda la possibilità di fornire un output uditivo della visione della macchina tramite l&#39;utilizzo di un piezo, che può simulare il segnale acustico dei reali sensori di parcheggio.
