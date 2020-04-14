#include <SPI.h>
#include "RH_RF95.h"
 
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define MAXNUMEVENTS 840
 
// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 434.775
 
// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

String appName = "Clock Transmitter V3";
byte transAddr = 23;
unsigned long period = 70000;
byte radiopacket[2];
unsigned long newTime = 0;
unsigned long time = 0;
byte timeLine[MAXNUMEVENTS];
int ievent = 0;
int numEvents = 14;
int sigPower = 3;
boolean goodCommand = false;
boolean validData = false;
 
void setup() 
{
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(9, OUTPUT);
  digitalWrite(11, LOW);
  digitalWrite(12, LOW);
  digitalWrite(9, LOW);
  
  Serial.begin(9600);
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
void loop() 
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
    checkSerial();
    rf95.setModeTx();
  }
}
void checkSerial()
{
  if (Serial.available())
  {
    String serialRead = readSerial();
    goodCommand = false;
    if (serialRead.lastIndexOf("aboutGet")      > -1) 
    {
      echoStringSetting("aboutGet", appName);
      goodCommand = true;
    }
    if (serialRead.lastIndexOf("powerSet")      > -1) 
    {
      sigPower = newIntSetting("powerSet", serialRead, 0, 23);
      rf95.setTxPower(sigPower, false);
      printStringToSerial("Success: " + serialRead + "\n");
      goodCommand = true;
    }
    if (serialRead.lastIndexOf("timelineSet")   > -1) 
    {
      validData = readTimeline(serialRead);
      if (validData)
      {
        printStringToSerial("Success: " + serialRead + "\n");
      }
      else
      {
        printStringToSerial("Error Bad Data: " + serialRead + "\n");
      }
      goodCommand = true;
    }
    if (serialRead.lastIndexOf("addressSet")      > -1) 
    {

      transAddr = (byte) newIntSetting("addressSet", serialRead, 0, 255);
      radiopacket[0] = transAddr;
      printStringToSerial("Success: " + serialRead + "\n");
      goodCommand = true;
    }
    if (serialRead.lastIndexOf("freqSet")      > -1) 
    {
      float freq = newFloatSetting("freqSet", serialRead);
      freq = 1000000.0 / freq;
      freq = freq / 4;
      int ifreq = freq * 4;
      period = (unsigned long) ifreq;
      printStringToSerial("Success: " + serialRead + "\n");
      goodCommand = true;
    }
    if (!goodCommand)
    {
      printStringToSerial("Error: " + serialRead + "\n");
      delay(100);
      Serial.end();
      delay(1);
      Serial.begin(9600);
    }
  }
  
}
String readSerial()
{
  String inputString = "";
  while(Serial.available() > 0)
  {
    char lastRecvd = Serial.read();
    delay(1);
    if (lastRecvd == '\n'){return inputString;} else {inputString += lastRecvd;}
  }
  return inputString;
}
void echoStringSetting(String getParse, String setting)
{
  printStringToSerial(getParse + " " + setting + "\n");
}
void echoIntSetting(String getParse, int setting)
{
  printStringToSerial(getParse + " " + String(setting) + "\n");
}
void printStringToSerial(String inputString)
{
  Serial.print(inputString);
}
int newIntSetting(String setParse, String input, int low, int high)
{
   String newSettingString = input.substring(input.lastIndexOf(setParse) + setParse.length() + 1,input.length());
   int newSetting = constrain(newSettingString.toInt(),low,high);
   return newSetting;
}
float newFloatSetting(String setParse, String input)
{
   String newSettingString = input.substring(input.lastIndexOf(setParse) + setParse.length() + 1,input.length());
   char floatbuf[32];
   newSettingString.toCharArray(floatbuf, sizeof(floatbuf));
   return atof(floatbuf);
}
boolean readTimeline(String serialRead)
{
  String arg;
  String remainder;
  int i1;
  remainder = serialRead;
  remainder.trim();
  i1 = remainder.indexOf(" ");
  if (i1 < 0) return false;
  remainder = remainder.substring(i1);
  remainder.trim();
  numEvents = 0;
  while (i1 > 0)
  {
    i1 = remainder.indexOf(" ");
    if (i1 > 0) 
    {
      arg = remainder.substring(0, i1);
      remainder = remainder.substring(i1);
      remainder.trim();
    }
    if (i1 < 0) 
    {
      arg = remainder;
      remainder = "";
    }
    arg.trim();
    timeLine[numEvents++] = (byte) arg.toInt();
    if (numEvents > (MAXNUMEVENTS - 2) )
    {
       ievent = 0;
       return true;
    }
//    Serial.print(numEvents); Serial.print("  "); Serial.println(timeLine[numEvents - 1]);
  }
  ievent = 0;
  return true;
}
