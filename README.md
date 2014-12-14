Progetto per irrigare un balcone con arduino ethernet

Arduino ethernet
Sensore Flusso acqua
Sensore di livello
Shield proto board
Modulo relay
Micro Sd da 2 GB
Contenitore stagno per l'elettronica
Contenitore di recupero in plastica capacità 60 litri
Pompa ad immersione alimentata a 12 volt
Pannellino solare da 5 watt
Batteria da 40 ampere
Impianto di irrigazione a goccia
Piante felici che non mi dimentico più di annafiarle

La scheda arduino ethernet integra a bordo un arduino+ethernet+microsd.

Quando la scheda si accende tramite i servizi di Ntp si aggiorna l'orario dal mio router che fà da server Ntp, visto che il timer interno di arduino è poco preciso ogni ora riaggiorno l'ora con NTP, nella eeprom dell'arduino sono salvati gli orari di inizio irrigazione e il tempo di irrigazione in minuti, con un massimo di 8 fascie orarie, sulla scheda microsd salvo i log di quantità e orario delle irrigazioni, il tutto è accessibile tramite un servizio telnet implementato sull'arduino con un menù piuttosto spartano:

ARDUINO IRRIGATORE
1=Stato modulo
2=Programmazione
3=Salva su EEPROM
4=Visualizza log
5=Cancella log
99=Esce

questo ad esempio è l'output richiamando la voce 1 del menù

ARDUINO IRRIGATORE
DATA/ORA 12/12/2014 17:36:12;
POMPA SPENTA
LIVELLO ACQUA NORMALE
0=Menu precedente

questo è un estratto del log salvato su microsd

12/12/2014 08:40:00;Pump On
12/12/2014 08:45:01;Pump Off
12/12/2014 08:45:01;Erogati 17649 cl Lvl Normale


come vedi ho qualche problema nel misurare correttamente la quantità erogata, 17 litri in 5 minuti sarebbe davvero un record e le piante sarebbero annegate.

In futuro vorrei implementare un servizio mail che mi avvisa quando l'acqua sta finendo, un sensore di pioggia che nel caso sospenda l'irrigazione, e magari passare l'interfaccia telnet a una pagina web, l'ostacolo più grosso al momento è la memoria dell'arduino, disponibile per l'applicativo, già adesso sono praticamente al limite, infatti stò pensando di affiancare l'arduino con un modulino linux embedded tipo raspberry o similari.

