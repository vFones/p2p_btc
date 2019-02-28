# retiBTC
Progetto esame di reti: traccia 1 - Bitcoin

A.A. 2018 - 2019

Si vuole realizzare un sistema per la gestione di una criptovaluta basato su una rete P2P. Il sistema è basato sulla gestione di una blockchain, ovvero una sequenza di blocchi in cui ogni blocco contiene una transazione.
Il sistema si compone di due tipi di nodi: NodiN e NodiW. I NodiN creano la rete P2P e gestiscono la blockchain. Inoltre stampano la blockchain ogni volta che viene aggiunto un blocco: (blocco1)− > (blocco2)− > (blocco3)− > (blocco4). I NodiW gestiscono i wallet (portafogli virtuali) che consentono di inviare e ricevere pagamenti. Ad ogni nuovo pagamento inviato o ricevuto il nodo stampa la transazione ed il totale del portafogli.
Si utilizzi il linguaggio C o Java su piattaforma UNIX utilizzando i socket per la comunicazione tra processi. Corredare l’implementazione di adeguata documentazione.
