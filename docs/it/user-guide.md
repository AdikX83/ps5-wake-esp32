# PS5 Wake – guida all’uso

PS5 Wake è una piccola scheda ESP32-C6 che accende la tua PlayStation 5 quando riceve un pacchetto Wake-on-LAN o
quando premi il suo pulsante. Via Bluetooth si presenta come il tuo controller DualSense associato, proprio come la
pressione del tasto PS. Funziona dalla modalità riposo e con la console completamente spenta.

## Cosa ti serve

- Una scheda ESP32-C6 con PS5 Wake (testata: ESP32-C6-DevKitM-1), alimentata da un qualsiasi caricatore USB.
- Un controller DualSense (o DualSense Edge) già associato alla PS5.
- Un computer e un cavo USB **dati**, una sola volta, per installare il firmware e leggere gli indirizzi del
  controller.
- Una rete Wi-Fi a 2,4 GHz.

## 1. Installa il firmware

Apri l’installer web (link nel README del progetto) in Chrome o Edge, collega la scheda con un cavo USB e fai clic su
**Install**. Un nuovo dispositivo si illumina di **viola**. Avvia da solo la rete del pannello di configurazione.

## 2. Apri il pannello di configurazione

1. Tieni premuto il pulsante **BOOT** per 5–20 secondi e rilascialo. Il LED diventa **rosso**. Su un dispositivo nuovo il pannello è già attivo (LED viola): salta questo passaggio.
2. Sul computer a cui collegherai il controller, connettiti alla rete Wi-Fi **`PS5-WAKE-xxxx`**, password
   **`ps5wol-setup`**.
3. Nel browser apri **`https://192.168.4.1`** (oppure `https://ps5-wake-xxxx.local`). Il browser avvisa che la connessione non è privata, perché l’ESP32 usa un proprio certificato: scegli **Avanzate → Procedi** (i nomi dipendono dal browser).
4. Scegli la lingua in alto nella pagina.

## 3. Leggi gli indirizzi del controller

1. Collega il DualSense al computer con il cavo USB.
2. Nella sezione **Bluetooth** del pannello scarica il lettore di indirizzi:
   - **Windows:** `ps5-wake-reader.cmd`, avvialo con un doppio clic. Se Windows segnala un autore sconosciuto,
     scegli **Ulteriori informazioni → Esegui comunque**.
   - **macOS:** esegui `pip3 install hidapi`, poi `python3 ps5_wake_reader.py`.
   - **Linux:** `sudo python3 ps5_wake_reader.py` (nessun pacchetto aggiuntivo).
3. Il lettore mostra gli indirizzi Bluetooth della console e del controller e copia un **codice di configurazione**
   (`PS5WAKE:…`) negli appunti. Incollalo nel campo **Codice di configurazione del lettore**: entrambi gli indirizzi
   vengono compilati.

Gli indirizzi cambiano solo se associ di nuovo il controller alla console.

## 4. Completa la configurazione

1. Scegli la rete Wi-Fi con **Cerca**, inserisci la password e scegli il paese.
2. Inserisci una **nuova password per la rete del pannello** (almeno 8 caratteri). È obbligatoria perché la password
   predefinita è pubblica.
3. Fai clic su **Salva e riavvia**. Il pannello si chiude, il LED lampeggia in **blu** durante la connessione e
   diventa **verde** quando è pronto.

## 5. Configura il Wake-on-LAN

- Nell’app Wake-on-LAN (o nella pagina WoL del router) usa l’**indirizzo MAC dell’ESP32** mostrato nel pannello, la
  porta UDP **9** e l’**indirizzo di broadcast** della rete (ad es. `192.168.1.255`).
- Riserva un indirizzo IP fisso per l’ESP32 nel router.
- Se l’app o il router usano già il **MAC di rete della PS5 stessa**, puoi inserirlo nel pannello: l’ESP32 reagirà
  anche a quello. Nelle reti mesh (ad es. ASUS AiMesh) questi pacchetti potrebbero non raggiungere l’ESP32; in tal
  caso invia il WoL al MAC dell’ESP32.

## Uso quotidiano

- **Stato della console:** il punto sull’icona di accensione in alto nel pannello è verde quando la PS5 è accesa, arancione in modalità riposo e rosso quando è spenta o non raggiungibile (grigio: non ancora rilevata). La modalità riposo viene rilevata solo se la console resta connessa a internet.
- **Svegliare la PS5:** invia un pacchetto Wake-on-LAN o premi brevemente **BOOT**. Il LED lampeggia una volta in
  bianco, lampeggia in ciano durante il tentativo, poi **3 volte in verde** (la console ha risposto) o **3 volte in
  rosso** (nessuna risposta).
- Tra un tentativo e l’altro passano almeno 30 secondi; una richiesta ignorata dà un lampo arancione.

## Il pulsante BOOT

| Durata della pressione | LED durante la pressione | Risultato |
|---|---|---|
| meno di 1 s | – | svegliare la PS5 |
| 1–5 s | bianco | niente (annulla) |
| 5–20 s, poi rilascia | rosso (lampeggia da 15 s) | aprire il pannello di configurazione |
| 20 s | viola, lampeggio rapido | cancellare tutte le impostazioni e riavviare |

Tenere premuto BOOT mentre si collega l’alimentazione avvia una modalità provvisoria senza Wi-Fi né Bluetooth (LED
giallo).

## Colori del LED

| Colore | Comportamento | Significato |
|---|---|---|
| viola | fisso | non configurato – pannello di configurazione attivo |
| blu | lampeggio lento | connessione al Wi-Fi |
| arancione | lampeggio lento | Wi-Fi configurato ma non connesso da oltre 30 s |
| verde (attenuato) | fisso | pronto (disattivabile nel pannello) |
| rosso | fisso | pannello di configurazione attivo |
| giallo | fisso | modalità provvisoria |
| bianco | un lampo | Wake-on-LAN ricevuto o BOOT premuto |
| ciano | lampeggio rapido | tentativo di risveglio in corso |
| verde | tre lampi | la console ha risposto |
| rosso | tre lampi | la console non ha risposto |
| arancione | un lampo | richiesta ignorata (tentativo in corso o pausa di 30 s) |

## Il pannello di configurazione

- Il pannello e la sua rete si spengono dopo il salvataggio, dopo **Chiudi senza modifiche** o dopo 10 minuti senza
  azioni. **Estendi (+10 min)** dà più tempo.
- **Spegni completamente il LED**: Nessun colore, lampeggio o lampo. Solo mentre tieni premuto BOOT il LED mostra ancora la durata della pressione.
- **Nome del dispositivo (facoltativo)**: Ad es. Soggiorno. Usato nell’indirizzo del pannello (`soggiorno.local`), in Home Assistant e nel titolo della pagina: utile con più dispositivi.
- **Accesso dalla rete di casa**: Attivo per impostazione predefinita. La pagina resta disponibile nella rete di casa su `<IP dell’ESP32>` e `ps5-wake-xxxx.local`: accedi con la password del pannello (nome utente admin, se non l’hai cambiato). La rete Wi-Fi del pannello non viene avviata; tieni premuto BOOT 5 s per aprirla.
- **Connessione cifrata (HTTPS)**: Consigliato. L’ESP32 crea un proprio certificato, quindi alla prima visita il browser avvisa che la connessione non è privata: confronta l’impronta qui sotto e continua. Senza HTTPS password e impostazioni viaggiano in chiaro nella rete. Il certificato viene rinnovato automaticamente prima della scadenza (l’ESP32 ottiene l’ora da internet); **Nuovo certificato** ne crea subito uno nuovo.
- **Sveglia la PS5 ora** prova il risveglio dal pannello.

## Risoluzione dei problemi

- **LED arancione lampeggiante:** password Wi-Fi errata, rete solo a 5 GHz o segnale debole. Apri il pannello,
  controlla il Wi-Fi e avvicina l’ESP32 al router. L’ESP32 si riavvia da solo se resta offline per 2 minuti.
- **Dopo l’invio del Wake-on-LAN non succede nulla (nessun lampo bianco):** il pacchetto non raggiunge l’ESP32. Usa
  il MAC dell’ESP32, l’indirizzo di broadcast e la porta 9; verifica che l’ESP32 non sia su una rete ospiti.
- **Lampi rossi dopo un tentativo:** controlla gli indirizzi Bluetooth (rileggili se hai riassociato il controller) e
  avvicina l’ESP32 alla console.
- **Il lettore non trova il controller:** usa un cavo dati (non solo di ricarica) e collega il controller
  direttamente al computer.
- **Il browser avvisa del certificato:** è normale, il pannello è cifrato con il certificato dell’ESP32. Prosegui alla pagina; il pannello mostra l’impronta del certificato.
