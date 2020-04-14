#include <SPI.h>
#include "RH_RF95.h"

#define BAUD_RATE 9600

#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define MAXNUMEVENTS 840
 
// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 434.775
 
// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);
byte transAddr = 23;
unsigned long period = 70000;
byte radiopacket[2];
unsigned long newTime = 0;
unsigned long time = 0;
byte timeLine[MAXNUMEVENTS];
int ievent = 0;
int numEvents = 14;
int sigPower = 3;

struct TransmitData
{
};
struct ReceiveData
{
  int loopDelay = 2000;
};

void setupPins()
{
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
  digitalWrite(11, LOW);
  digitalWrite(12, LOW);
  delay(100);
 
  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
  rf95.init();
  rf95.setFrequency(RF95_FREQ);
//rf95.setModemConfig(RH_RF95::ModemConfigChoice::Bw125Cr45Sf128); 
  rf95.setModemConfig(RH_RF95::ModemConfigChoice::Bw500Cr45Sf128); 
  rf95.setModeTx();
  rf95.setTxPower(sigPower, false);
  radiopacket[0] = transAddr;
  radiopacket[1] = timeLine[0];
  int ii = 0;
  for (ii = 0; ii < numEvents; ++ii) timeLine[ii] = 1;
  ievent = 0;
}
void processNewSetting(TransmitData* tData, ReceiveData* rData, ReceiveData* newData)
{
  rData->loopDelay = newData->loopDelay;
 }
boolean processData(TransmitData* tData, ReceiveData* rData)
{
  newTime = micros();
  if ((newTime - time) > period)
  {
    rf95.send((uint8_t *)radiopacket, 2);
    digitalWrite(11, HIGH);
    digitalWrite(12, HIGH);
    delayMicroseconds(2000);
    digitalWrite(11, LOW);
    digitalWrite(12, LOW);
    if (ievent == numEvents) ievent  = 0;
    radiopacket[1] = timeLine[ievent];
    ++ievent;
    time = newTime;
    rf95.setModeTx();
  }
  return true;
}

const int microLEDPin = 13;
const int commLEDPin = 13;
boolean commLED = true;

struct TXinfo
{
  int cubeInit = 1;
  int newSettingDone = 0;
};
struct RXinfo
{
  int newSetting = 0;
};

struct TX
{
  TXinfo txInfo;
  TransmitData txData;
};
struct RX
{
  RXinfo rxInfo;
  ReceiveData rxData;
};
TX tx;
RX rx;
ReceiveData settingsStorage;

int sizeOfTx = 0;
int sizeOfRx = 0;

void setup()
{
  setupPins();
  pinMode(microLEDPin, OUTPUT);    
  pinMode(commLEDPin, OUTPUT);  
  digitalWrite(commLEDPin, commLED);
//  digitalWrite(microLEDPin, commLED);

  sizeOfTx = sizeof(tx);
  sizeOfRx = sizeof(rx);
  Serial1.begin(BAUD_RATE);
  delay(1000);
}
void loop()
{
  boolean goodData = false;
  goodData = processData(&(tx.txData), &settingsStorage);
  if (goodData)
  {
    tx.txInfo.newSettingDone = 0;
    if(Serial1.available() > 0)
    { 
      commLED = !commLED;
      digitalWrite(commLEDPin, commLED);
      Serial1.readBytes((uint8_t*)&rx, sizeOfRx);
      
      if (rx.rxInfo.newSetting > 0)
      {
        processNewSetting(&(tx.txData), &settingsStorage, &(rx.rxData));
        tx.txInfo.newSettingDone = 1;
        tx.txInfo.cubeInit = 0;
      }
    }
    Serial1.write((uint8_t*)&tx, sizeOfTx);
  }
  
}
