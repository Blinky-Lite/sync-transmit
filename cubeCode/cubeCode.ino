#include <SPI.h>
#include "RH_RF95.h"

#define BAUD_RATE 9600

#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define MAXNUMEVENTS 100
// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 434
 
// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);
byte radiopacket[2];
unsigned long newTime = 0;
unsigned long lastWriteTime = 0;
int ievent = 0;
int channelPin[] = {11,10,6,5};
boolean channelHigh[] = {false, false, false, false};

boolean pin12Value = true;

struct TransmitData
{
};
struct ReceiveData
{
  byte timeLine[MAXNUMEVENTS];
  int numEvents = 10;
  int sigPower = 3;
  byte transAddr = 23;
  unsigned long intervalUs = 100000;
  unsigned long channelBeginTime[4];
  unsigned long channelEndTime[4];
  byte channelStateMask[4];
};

void setupPins(TransmitData* tData, ReceiveData* rData)
{
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  pinMode(12, OUTPUT);
  digitalWrite(12, pin12Value);
  for( int ic = 0; ic < 4; ++ic ) pinMode(channelPin[ic], OUTPUT);
  for( int ic = 0; ic < 4; ++ic ) digitalWrite(channelPin[ic], LOW);

  delay(100);
 
  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
  rf95.init();
  rf95.setFrequency(RF95_FREQ);
  rf95.setModemConfig(RH_RF95::ModemConfigChoice::Bw500Cr45Sf128); 
  rf95.setModeTx();
  rf95.setTxPower(rData->sigPower, false);
  radiopacket[0] = rData->transAddr;
  radiopacket[1] = rData->timeLine[0];
  for (int ii = 0; ii < rData->numEvents; ++ii) rData->timeLine[ii] = 0;
  for (int ic = 0; ic < 4; ++ic)
  {
    rData->channelBeginTime[ic] = 1000;
    rData->channelEndTime[ic] = 2000;
    rData->channelStateMask[ic] = 0;
  }
  ievent = 0;
  
// Test data
  rData->timeLine[0] = 1;
  rData->channelStateMask[0] = 1;
  rData->channelBeginTime[0] = 10200;
  rData->channelEndTime[0] = 12200;

}
void processNewSetting(TransmitData* tData, ReceiveData* rData, ReceiveData* newData)
{
  rData->transAddr = newData->transAddr;
  rData->numEvents = newData->numEvents;
  for (int ii = 0; ii < rData->numEvents; ++ii) rData->timeLine[ii] = newData->timeLine[ii];
  for (int ic = 0; ic < 4; ++ic)
  {
    rData->channelBeginTime[ic] = newData->channelBeginTime[ic];
    rData->channelEndTime[ic] = newData->channelEndTime[ic];
    rData->channelStateMask[ic] = newData->channelStateMask[ic];
  }

  if (newData->sigPower != rData->sigPower)
  {
    rData->sigPower = newData->sigPower;
    rf95.setTxPower(rData->sigPower, false);
  }
  radiopacket[0] = newData->transAddr;
  radiopacket[1] = newData->timeLine[0];
  ievent = 0;
}
boolean processData(TransmitData* tData, ReceiveData* rData)
{
  newTime = micros();
  unsigned long deltaT = newTime - lastWriteTime;
  boolean timeLineRestart = false;
  if (deltaT > rData->intervalUs)
  {
    radiopacket[1] = rData->timeLine[ievent];
    rf95.send((uint8_t *)radiopacket, 2);
    pin12Value = !pin12Value;
    digitalWrite(12, pin12Value);

    ++ievent;
    if (ievent == rData->numEvents) ievent  = 0;

    lastWriteTime = newTime;
    deltaT = 0;
    timeLineRestart =  true;
  }
  for (int ic = 0; ic < 4; ++ic)
  {
    channelHigh[ic] = false;
    if ( (radiopacket[1] & rData->channelStateMask[ic]) > 0)
    {
      if ((rData->channelBeginTime[ic] <= deltaT) && (deltaT <= rData->channelEndTime[ic]))
      {
        channelHigh[ic] = true;
      }
    }
    digitalWrite(channelPin[ic], channelHigh[ic]);
  }  
  return timeLineRestart;
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
  setupPins(&(tx.txData), &settingsStorage);
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
