/*
  Irrigatore
 
  Gestione dell'irrigazione del balcone di casa
  utilizzato Arduino Ethernet. 
 
 * Ethernet collegata ai pins 10, 11, 12, 13
 * SD collegata al pin 4
 * Sensore livello acqua collegato al pin 7
 * Contatore acqua collegato al pin 2
 * Output attivazione Relay collegato al pin 8
 
 creato il 19 Maggio 2013
 da Paolo Coatti
 
 */

#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>
#include <EEPROM.h>
#include <EthernetUdp.h>
#include <Time.h>
#include <avr/pgmspace.h>

prog_char msg_0[] PROGMEM = "FREE RAM ";
prog_char msg_1[] PROGMEM = "Card Init";
prog_char msg_2[] PROGMEM = "Card Fail";
prog_char msg_3[] PROGMEM = "Ntp Success";
prog_char msg_4[] PROGMEM = "Ntp Fail";
prog_char msg_5[] PROGMEM = "Default Parameter";
prog_char msg_6[] PROGMEM = "EEPROM Parameter";
prog_char msg_7[] PROGMEM = "UDP Extra recv";
prog_char msg_8[] PROGMEM = "Pump On";
prog_char msg_9[] PROGMEM = "Pump Off";
prog_char msg_10[] PROGMEM = "EEPROM Saved";
prog_char msg_11[] PROGMEM = "ERROR OPEN IRRIGA.TXT";
prog_char msg_12[] PROGMEM = "Erogati ";

#define FREE_RAM 0
#define CARD_INIT 1
#define CARD_FAIL 2
#define NTP_SUCC 3
#define NTP_FAIL 4
#define DEF_PARM 5
#define EEPR_PARM 6
#define UDP_EXTRA 7
#define PUMP_ON 8
#define PUMP_OFF 9
#define EEPROM_SAVE 10
#define ERROR_OPEN 11
#define CL_EROGATI 12

PROGMEM const char *msg_table[] = 
{  
  msg_0, 
  msg_1, 
  msg_2, 
  msg_3, 
  msg_4, 
  msg_5, 
  msg_6, 
  msg_7, 
  msg_8, 
  msg_9, 
  msg_10, 
  msg_11,
  msg_12 };

prog_char header[] PROGMEM = "ARDUINO IRRIGATORE";
prog_char footer[] PROGMEM = "Scelta=";

prog_char menu0_0[] PROGMEM = "1=Stato modulo";
prog_char menu0_1[] PROGMEM = "2=Programmazione";
prog_char menu0_2[] PROGMEM = "3=Salva su EEPROM";
prog_char menu0_3[] PROGMEM = "4=Visualizza log";
prog_char menu0_4[] PROGMEM = "5=Cancella log";
prog_char menu0_5[] PROGMEM = "99=Esce";

// Then set up a table to refer to your strings.

PROGMEM const char *menu0_table[] = 
{  
  header, 
  menu0_0,
  menu0_1,
  menu0_2,
  menu0_3,
  menu0_4,
  menu0_5,
  footer };

prog_char menu1_0[] PROGMEM = "DATA/ORA ";
prog_char menu1_1[] PROGMEM = "POMPA ";
prog_char menu1_2[] PROGMEM = "LIVELLO ACQUA ";
prog_char menu1_3[] PROGMEM = "0=Menu precedente";

PROGMEM const char *menu1_table[] = 
{  
  header, 
  menu1_0,
  menu1_1,
  menu1_2,
  menu1_3 };

prog_char menu2_0[] PROGMEM = "0=Menu precedente";
prog_char menu2_1[] PROGMEM = "1=Modifica riga";
prog_char menu2_2[] PROGMEM = "2=Inserisci riga";
prog_char menu2_3[] PROGMEM = "3=Cancella riga";
prog_char menu2_4[] PROGMEM = "Numero riga da modificare";
prog_char menu2_5[] PROGMEM = "Numero riga da inserire";
prog_char menu2_6[] PROGMEM = "Numero riga da cancellare";
prog_char menu2_7[] PROGMEM = "Ora Inizio";
prog_char menu2_8[] PROGMEM = "Minuto Inizio";
prog_char menu2_9[] PROGMEM = "Tempo minuti";

PROGMEM const char *menu2_table[] = 
{  
  header, 
  menu2_0,
  menu2_1,
  menu2_2,
  menu2_3,
  menu2_4,
  menu2_5,
  menu2_6,
  menu2_7,
  menu2_8,
  menu2_9 };

prog_char menu3_0[] PROGMEM = "0=Menu precedente";

PROGMEM const char *menu3_table[] = 
{  
  header, 
  menu3_0 };

char buffer[50];

#define MAXTIME 4

// MAC address e IP address
byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0x6C, 0xE6 };
IPAddress ip(192,168,0, 177);
IPAddress gateway(192, 168, 0, 244); 

/* ******** impostazione NTP Server ******** */
/* ntp.ngi.it NTP server */
IPAddress timeServer(88, 149, 128, 123);
 
/* impostazione localtime GMT + 2 (ora legale) */
const long timeZoneOffset = 7200L;  
 
/* impostazione periodo di sincronizzazione 1 ora */
unsigned int ntpSyncTime = 3600;        
 
 
// Porta UDP per i pacchetti NTP
unsigned int localPort = 8888;
// Dimensione Pacchetto NTP
const int NTP_PACKET_SIZE= 48;      
// Buffer per contenere il Pacchetto NTP
byte packetBuffer[NTP_PACKET_SIZE];  
// Instanza UDP sulla rete ethernet per ricevere il pacchetto UDP
EthernetUDP Udp;
// telnet defaults to port 23
EthernetServer server(23);
EthernetClient client;
// Orario dell'ultima sincronizzazione
unsigned long ntpLastUpdate = 0;
// Orario di inizio irrigazione
unsigned long StartTempo = 0;
unsigned int TempoIrrigazione = 0;
unsigned int TempoAllarmeLvl = 0;

const int chipSelect = 4;
const int relay = 8;
const int hallsensor = 2;    //The pin location of the sensor
const int waterLvl = 7;

volatile unsigned long rpmcount; //measuring the rising edges of the signal
unsigned long Calc;                               

int AlrmLvl = 0;
int SDCard = 0;
int MenuLevel = 0;
int SubMenuLevel = 0;

byte hhbgn[MAXTIME];  // ora inizio irrigazione
byte mmbgn[MAXTIME];  // minuto inizio irrigazione
byte tempo[MAXTIME];  // tempo in minuti irrigazione

byte RigaSel = 0;

boolean alreadyConnected = false; // whether or not the client was connected previously

String Scelta = String(10);

void setup() {
  Serial.begin(9600);
  int wn;
  for (wn = 0; wn < MAXTIME; wn++) {
      hhbgn[wn] = 0;
      mmbgn[wn] = 0;
      tempo[wn] = 0;
  }
   
  // Attiva la rete ethernet e la socket UDP:
  Ethernet.begin(mac, ip, gateway, gateway);
  Udp.begin(localPort);
 
  pinMode(hallsensor, INPUT);
  pinMode(relay, OUTPUT);
  pinMode(waterLvl, INPUT);
  digitalWrite(waterLvl, HIGH);
  rpmcount = 0;
  WriteLog(FREE_RAM);
 
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect))
    WriteLog(CARD_FAIL);
  else {
    SDCard = 1;
    WriteLog(CARD_INIT);
  }
  WriteLog(FREE_RAM);
  
  // Imposta l'orologio locale tramite NTP
  int trys=0;
  while(!getTimeAndDate() && trys<3) {
    trys++;
  }
  if(trys<3){
    WriteLog(NTP_SUCC);
  }
  else{
    WriteLog(NTP_FAIL);
  }

  if (EEPROM.read(0) != 1) {
    hhbgn[0] = 6;
    mmbgn[0] = 45;
    tempo[0] = 15;
    hhbgn[1] = 19;
    mmbgn[1] = 45;
    tempo[1] = 15;
    WriteLog(DEF_PARM);
  }
  else {
    readEEPROM();
    WriteLog(EEPR_PARM);
  }
  Scelta = "";
}

void rpm_fun()
 {
   rpmcount++;
   //Each rotation, this interrupt function is run
 }

// Returns the number of bytes currently free in RAM
static int freeRAM(void) 
{
 extern int  __bss_end;
 extern int* __brkval;
 int free_memory;
 if (reinterpret_cast<int>(__brkval) == 0) {
   free_memory = reinterpret_cast<int>(&free_memory) - reinterpret_cast<int>(&__bss_end);
 } else  {
   free_memory = reinterpret_cast<int>(&free_memory) - reinterpret_cast<int>(__brkval);
 }
 return free_memory;
}

void readEEPROM() {
  int wn;
  for (wn = 0; wn < MAXTIME; wn++) {
      hhbgn[wn] = EEPROM.read(wn + 1);
      mmbgn[wn] = EEPROM.read(wn + MAXTIME + 1);
      tempo[wn] = EEPROM.read(wn + (MAXTIME * 2) + 1);
  }
}

void saveEEPROM() {
  int wn;
  EEPROM.write(0, 1);
  for (wn = 0; wn < MAXTIME; wn++) {
      EEPROM.write(wn + 1, hhbgn[wn]);
      EEPROM.write(wn + MAXTIME + 1, mmbgn[wn]);
      EEPROM.write(wn + (MAXTIME * 2) + 1, tempo[wn]);
  }
}

int getTimeAndDate() {
  int flag = 0;
  int packetSize = 0;
  
  do {
     packetSize = Udp.parsePacket();
     if(packetSize)
     {
        WriteLog(UDP_EXTRA);
        Udp.read(packetBuffer,NTP_PACKET_SIZE);  // read the packet into the buffer
     }
   } while (packetSize);
   
   sendNTPpacket(timeServer);
   delay(100);
   if (Udp.parsePacket()){
     Udp.read(packetBuffer,NTP_PACKET_SIZE);
     unsigned long highWord, lowWord, epoch;
     highWord = word(packetBuffer[40], packetBuffer[41]);
     lowWord = word(packetBuffer[42], packetBuffer[43]);  
     epoch = highWord << 16 | lowWord;  // epoch sono i secondi dal 01/01/1900
     epoch = epoch - 2208988800 + timeZoneOffset; // trasforma epoch in secondi dal 01/01/1970 
     flag=1;
     setTime(epoch);
     ntpLastUpdate = now();
   }
   return flag;
}
 
// invia il pacchetto di richiesta ora via NTP
unsigned long sendNTPpacket(IPAddress& address)
{
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;
  packetBuffer[1] = 0;
  packetBuffer[2] = 6;
  packetBuffer[3] = 0xEC;
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;                  
  Udp.beginPacket(address, 123);
  Udp.write(packetBuffer,NTP_PACKET_SIZE);
  Udp.endPacket();
}
 
void WriteLog(int msg) {
  char dspdate[22];
  char s1[16];
  
  sprintf(dspdate, "%02d/%02d/%04d %02d:%02d:%02d;",day(), month(), year(), hour(), minute(), second());
  strcpy_P(buffer, (char*)pgm_read_word(&(msg_table[msg])));

  switch(msg) {
   case FREE_RAM:
    sprintf(s1, "%d", freeRAM());
    strcat (buffer, s1);
    break;
   case CL_EROGATI:
    sprintf(s1, "%ld cl", Calc);
    strcat (buffer, s1);
    if (AlrmLvl == 1)
      strcat(buffer, " Lvl Allarme");
    else
      strcat(buffer, "Lvl Normale");
    break;
   default:
    break;
  };

  if (SDCard == 1) {
    File dataFile = SD.open("irriga.txt", FILE_WRITE);

    // if the file is available, write to it:
    if (dataFile) {
      dataFile.print(dspdate);
      dataFile.println(buffer);
      dataFile.close();
    // print to the serial port too:
    }  
    // if the file isn't open, pop up an error:
    else {
      strcpy_P(buffer, (char*)pgm_read_word(&(msg_table[ERROR_OPEN])));
      Serial.println(buffer);
      strcpy_P(buffer, (char*)pgm_read_word(&(msg_table[msg])));
    }
  } 
  Serial.print(dspdate);
  Serial.println(buffer);
}

void DspMenu() {
  char dspdate[32];
  int wn;
  switch(MenuLevel) {
    case 0:
      for (wn = 0; wn < 7; wn++) {
        strcpy_P(buffer, (char*)pgm_read_word(&(menu0_table[wn])));
        client.println(buffer);
      }
      break;
    case 1:
      strcpy_P(buffer, (char*)pgm_read_word(&(menu1_table[0])));
      client.println(buffer);
      sprintf(dspdate, "%02d/%02d/%04d %02d:%02d:%02d;",day(), month(), year(), hour(), minute(), second());
      strcpy_P(buffer, (char*)pgm_read_word(&(menu1_table[1])));
      client.print(buffer);
      client.println(dspdate);
      strcpy_P(buffer, (char*)pgm_read_word(&(menu1_table[2])));
      if (StartTempo != 0)
        strcat(buffer, "ACCESA");
      else
        strcat(buffer, "SPENTA");
      client.println(buffer);
      strcpy_P(buffer, (char*)pgm_read_word(&(menu1_table[3])));
      if (AlrmLvl == 1)
        strcat(buffer, "ALLARME");
      else
        strcat(buffer, "NORMALE");
      client.println(buffer);
      strcpy_P(buffer, (char*)pgm_read_word(&(menu1_table[4])));
      client.println(buffer);
      break;
    case 2:
      strcpy_P(buffer, (char*)pgm_read_word(&(menu2_table[0])));
      client.println(buffer);
      switch(SubMenuLevel) {
        case 0:
          for (wn = 0; wn < MAXTIME; wn++) {
            if (tempo[wn] != 0) {
              sprintf(dspdate, "%d) Orario %02d:%02d minuti %02d", wn+1, hhbgn[wn], mmbgn[wn], tempo[wn]);
              client.println(dspdate);
            }
          }  
          for (wn = 1; wn < 5; wn++) {
            strcpy_P(buffer, (char*)pgm_read_word(&(menu2_table[wn])));
            client.println(buffer);
          }
          break;
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
          strcpy_P(buffer, (char*)pgm_read_word(&(menu2_table[SubMenuLevel + 4])));
          client.println(buffer);
          break;
        default:
          break;
      };
      break;
    case 3:
      strcpy_P(buffer, (char*)pgm_read_word(&(menu3_table[0])));
      client.println(buffer);
      if (SDCard == 1) {
        File dataFile = SD.open("irriga.txt", FILE_READ);

        // if the file is available, read from it:
        if (dataFile) {
          while (dataFile.available()) {
            client.write(dataFile.read());
          }
          dataFile.close();
        }  
      }
      strcpy_P(buffer, (char*)pgm_read_word(&(menu3_table[1])));
      client.println(buffer);
      break;
    default:
      break;
  };
} 

void loop() {
    int wn;
    
    // Aggiorna l'orario tramite NTP
    if(now()-ntpLastUpdate > ntpSyncTime) {
      int trys=0;
      while(!getTimeAndDate() && trys<3){
        trys++;
      }
      if(trys<3){
        // WriteLog(NTP_SUCC);
      }
      else{
        // WriteLog(NTP_FAIL);
        ntpLastUpdate = now();
      }
    }

    if (StartTempo != 0) {
       if (now()-StartTempo > TempoIrrigazione) {
          digitalWrite(relay, LOW);
          StartTempo = 0;
          TempoIrrigazione = 0;
          WriteLog(PUMP_OFF);
          detachInterrupt(0);
          Calc = rpmcount * 333; //(Pulse frequency x 60) / 7.5Q, = flow rate in L/hour
          WriteLog(CL_EROGATI);
       } 
    }
    else {
       for (wn = 0; wn < MAXTIME; wn++) {
           if (mmbgn[wn] == minute() && hhbgn[wn] == hour() && tempo[wn] != 0)
               break;
       }
       if (wn < MAXTIME) {
          digitalWrite(relay, HIGH);
          StartTempo = now();
          TempoIrrigazione = tempo[wn] * 60;
          WriteLog(PUMP_ON);
          rpmcount = 0;
          attachInterrupt(0, rpm_fun, RISING);
       }
    }
  
    int livello = digitalRead(waterLvl);
    if (livello == HIGH && AlrmLvl == 1) {
      if (TempoAllarmeLvl == 0)
        TempoAllarmeLvl = now();
      if (now()-TempoAllarmeLvl > 10) {  
        AlrmLvl = 0;
        TempoAllarmeLvl = 0;
      }
    }
    if (livello == LOW && AlrmLvl == 0) {
      if (TempoAllarmeLvl == 0)
        TempoAllarmeLvl = now();
      if (now()-TempoAllarmeLvl > 10) {  
        AlrmLvl = 1;
        TempoAllarmeLvl = 0;
      }
    }
  
  // wait for a new client:
  client = server.available();

  // when the client sends the first byte, say hello:
  if (client) {
    if (!alreadyConnected) {
      // clead out the input buffer:
      client.flush();    
      MenuLevel = 0;
      DspMenu(); 
      alreadyConnected = true;
    } 

    if (client.available() > 0) {
      // read the bytes incoming from the client:
      char thisChar = client.read();
      if (isDigit(thisChar) && Scelta.length() <= 10) {
        Scelta += thisChar;
      }
      if (thisChar == '\n' ) {
        if (MenuLevel == 0) { 
          switch (Scelta.toInt()) {
            case 0:
              MenuLevel = 0;
              DspMenu();
              break;
            case 1:
              MenuLevel = 1;
              DspMenu();
              break;
            case 2:
              MenuLevel = 2;
              DspMenu();
              break;
            case 3:
              saveEEPROM();
              WriteLog(EEPROM_SAVE);
              DspMenu();
              break;
            case 4:
              MenuLevel = 3;
              DspMenu();
              break;
            case 5:
              if (SDCard == 1) {
                if (SD.remove("irriga.txt") == 1) {
                  WriteLog(FREE_RAM);
                }
              }
              DspMenu();
              break;
            case 99:
              client.stop();
              alreadyConnected = false;
              break;
            default:
              break;
          };
        }
        else if (MenuLevel == 2) {
          switch (SubMenuLevel) {
            case 0:
              switch (Scelta.toInt()) {
                case 0:
                  MenuLevel = 0;
                  DspMenu();
                  break;
                case 1:
                  SubMenuLevel = 1;
                  DspMenu();
                  break;
                case 2:
                  SubMenuLevel = 2;
                  DspMenu();
                  break;
                case 3:
                  SubMenuLevel = 3;
                  DspMenu();
                  break;
                default:
                  break;
              };
              break;
            case 1:
              RigaSel = Scelta.toInt() - 1;
              if (RigaSel >= MAXTIME || hhbgn[RigaSel] == 0) {
                SubMenuLevel = 0;
                DspMenu();
                break;
              }
              SubMenuLevel = 4;
              DspMenu();
              break;
            case 2:
              RigaSel = Scelta.toInt() - 1;
              if (RigaSel >= MAXTIME || hhbgn[RigaSel] != 0) {
                SubMenuLevel = 0;
                DspMenu();
                break;
              }
              SubMenuLevel = 4;
              DspMenu();
              break;
            case 3:
              RigaSel = Scelta.toInt() - 1;
              if (RigaSel >= MAXTIME || hhbgn[RigaSel] == 0) {
                SubMenuLevel = 0;
                DspMenu();
                break;
              }
              hhbgn[RigaSel] = 0;
              mmbgn[RigaSel] = 0;
              tempo[RigaSel] = 0;
              SubMenuLevel = 0;
              DspMenu();
              break;
            case 4:
              if (Scelta.toInt() > 23) {
                SubMenuLevel = 0;
                DspMenu();
                break;
              }
              hhbgn[RigaSel] = Scelta.toInt();
              SubMenuLevel = 5;
              DspMenu();
              break;
            case 5:
              if (Scelta.toInt() > 59) {
                SubMenuLevel = 0;
                DspMenu();
                break;
              }
              mmbgn[RigaSel] = Scelta.toInt();
              SubMenuLevel = 6;
              DspMenu();
              break;
            case 6:
              if (Scelta.toInt() == 0) {
                SubMenuLevel = 0;
                DspMenu();
                break;
              }
              tempo[RigaSel] = Scelta.toInt();
              SubMenuLevel = 0;
              DspMenu();
              break;
            default:
              break;
          };
        } 
        else {
          switch (Scelta.toInt()) {
            case 0:
              MenuLevel = 0;
              DspMenu();
              break;
            default:
              break;
          };
        }
        Scelta = "";
      }
    }
  }
  delay(50);
}

