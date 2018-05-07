#include <WiFi.h>
#include <WiFiUdp.h>
const char * networkName = "A1-7FB051";
const char * networkPswd = "hainz2015";

const char * udpAddress = "10.0.0.55";
const int udpPort = 1236;


//Are we currently connected?
boolean connected = false;

//The udp library class
WiFiUDP udp;


uint64_t chipid;  

//const int RCV_CHIP_SELECT = 5;
//Chip Select Pins 5,17,16,4 

const int RECEIVER_PINS[] = {5, 17, 16, 4};
int triggerThreshold[3];
#define RECEIVER_COUNT 4
int RCV_CHIP_SELECT = 5;
const int RCV_CLK = 18;
const int RCV_DATA = 19;


uint16_t frequencies[] = {5800, 5740, 0, 0, 0, 0, 0};
uint16_t f_register[] = {0x2984, 0x2906, 0x2a1f, 0x281D, 0, 0, 0};

uint16_t lastValue = 0;
bool increasing = true;
uint16_t tuningFrequency = 0;
uint16_t tunedFrequency = 0;
int val0=0;
int val1=0;
int val2=0;
int val3=0;

const char serialSeperator = ' ';
const char lineEnding = '\n';


struct {
  uint16_t volatile vtxFreq = 5800;
  // Subtracted from the peak rssi during a calibration pass to determine the trigger value
  uint16_t volatile calibrationOffset = 8;
  // Rssi must fall below trigger - settings.calibrationThreshold to end a calibration pass
  uint16_t volatile calibrationThreshold = 90;
  // Rssi must fall below trigger - settings.triggerThreshold to end a normal pass
  uint16_t volatile triggerThreshold = 40;
  uint8_t volatile filterRatio = 10;
  float volatile filterRatioFloat = 0.0f;
} settings[3];

struct {
  bool volatile calibrationMode = false;
  // True when the quad is going through the gate
  bool volatile crossing = false;
  // Current unsmoothed rssi
  uint16_t volatile rssiRaw = 0;
  // Smoothed rssi value, needs to be a float for smoothing to work
  float volatile rssiSmoothed = 0;
  // int representation of the smoothed rssi value
  uint16_t volatile rssi = 0;
  // rssi value that will trigger a new pass
  uint16_t volatile rssiTrigger;
  // The peak raw rssi seen the current pass
  uint16_t volatile rssiPeakRaw = 0;
  // The peak smoothed rssi seen the current pass
  uint16_t volatile rssiPeak = 0;
  // The time of the peak raw rssi for the current pass
  uint32_t volatile rssiPeakRawTimeStamp = 0;

  // variables to track the loop time
  uint32_t volatile loopTime = 0;

  //uint32_t volatile lastLoopTimeStamp = 0;
  uint32_t lastLoopTimeStamp[3];

} state[3];

struct {
  uint16_t volatile rssiPeakRaw;
  uint16_t volatile rssiPeak;
  uint32_t volatile timeStamp;
  uint8_t volatile lap;
} lastPass[3];


int workmode=0;





void setup() {
 
  Serial.begin(250000);

     connectToWiFi(networkName, networkPswd);

  // Initialize lastPass defaults
     for (int i=0; i <= 3; i++){
  settings[i].filterRatioFloat = settings[i].filterRatio / 1000.0f;
  state[i].rssi = 0;
  state[i].rssiTrigger = 0;
  lastPass[i].rssiPeakRaw = 0;
  lastPass[i].rssiPeak = 0;
  lastPass[i].lap = 0;
  lastPass[i].timeStamp = 0;

     }





  delay(500);
  pinMode(RCV_DATA, OUTPUT);
  pinMode(RCV_CLK, OUTPUT);
  //pinMode(RCV_CHIP_SELECT, OUTPUT);
  pinMode(5, OUTPUT);
 	pinMode(17, OUTPUT);
  pinMode(16, OUTPUT);
  pinMode(4, OUTPUT);
  



   chipid=ESP.getEfuseMac();//The chip ID is essentially its MAC address(length: 6 bytes).
  Serial.printf("ESP32 Chip ID = %04X",(uint16_t)(chipid>>32));//print High 2 bytes
  Serial.printf("%08X\n",(uint32_t)chipid);//print Low 4bytes.
delay(500);

RCV_CHIP_SELECT=5;
RCV_FREQ(f_register[3]); //LETZTE?
delay(500);
digitalWrite(5, HIGH);
delay(500);
RCV_CHIP_SELECT=17;
RCV_FREQ(f_register[3]); //Vorletzte
delay(500);
digitalWrite(17, HIGH);
delay(500);
RCV_CHIP_SELECT=16;
RCV_FREQ(f_register[3]);
delay(500);
digitalWrite(16, HIGH);
delay(500);
RCV_CHIP_SELECT=4;
RCV_FREQ(f_register[3]);
delay(500);
digitalWrite(4, HIGH);
delay(500);




}

void loop() {

if(workmode==0){
val0 = analogRead(36);    // read the input pin
val1 = analogRead(39);    // read the input pin
val2 = analogRead(34);    // read the input pin
val3 = analogRead(35);    // read the input pin

Serial.print(F("r "));
Serial.print(val0);
 Serial.print(serialSeperator);
Serial.print(val1);
 Serial.print(serialSeperator);
Serial.print(val2);
 Serial.print(serialSeperator);
Serial.print(val3);
Serial.print(lineEnding);

 if(connected){
    //Send a packet
    udp.beginPacket(udpAddress,udpPort);
    //udp.printf("Seconds since boot: %u", millis()/1000);
    udp.printf("%u %u %u %u", val0, val1, val2,val3);

    
    udp.endPacket();
    delay(25);
  }


}

      for (int i=0; i <= 3; i++){
if(workmode==3){

  state[i].calibrationMode=false;
     
}
if(workmode==4){
  state[i].calibrationMode=true;
}
if(workmode==5){
  state[i].rssiTrigger=0;
}
if(workmode==6){
  state[i].rssiTrigger=1;
}

if(workmode==7){

}
      }
if(workmode==1 or workmode==3 or workmode==4){
  // Calculate the time it takes to run the main loop

     for (int i=0; i <= 3; i++){


 
  //HELP HERE lastLoopTimeStamp[i] = state[i].lastLoopTimeStamp;
 // state[i].lastLoopTimeStamp = micros();
  //state[i].loopTime = state[i].lastLoopTimeStamp - lastLoopTimeStamp[i];

  state[i].rssiRaw = analogRead(39);
  state[i].rssiSmoothed = (settings[i].filterRatioFloat * (float)state[i].rssiRaw) + ((1.0f-settings[i].filterRatioFloat) * state[i].rssiSmoothed);
  //state.rssiSmoothed = state.rssiRaw;
  state[i].rssi = (int)state[i].rssiSmoothed;

  if (state[i].rssiTrigger > 0) {
    if (!state[i].crossing && state[i].rssi > state[i].rssiTrigger) {
      state[i].crossing = true; // Quad is going through the gate
      Serial.println("Crossing = True"+ i);
    }

    // Find the peak rssi and the time it occured during a crossing event
    // Use the raw value to account for the delay in smoothing.
    if (state[i].rssiRaw > state[i].rssiPeakRaw) {
       //Serial.println("rssiraw>rssipeak");
      state[i].rssiPeakRaw = state[i].rssiRaw;
      state[i].rssiPeakRawTimeStamp = millis();
    }

    if (state[i].crossing) {
      //int triggerThreshold[i] = settings[i].triggerThreshold;
      triggerThreshold[i] = settings[i].triggerThreshold;
      
      
       //Serial.println("state-crossing");
      // If in calibration mode, keep raising the trigger value
      if (state[i].calibrationMode) {
         Serial.println("in calibration" + i);
        state[i].rssiTrigger = _max(state[i].rssiTrigger, state[i].rssi - settings[i].calibrationOffset);
        // when calibrating, use a larger threshold
        triggerThreshold[i] = settings[i].calibrationThreshold;
      }

      state[i].rssiPeak = _max(state[i].rssiPeak, state[i].rssi);

      // Make sure the threshold does not put the trigger below 0 RSSI
      // See if we have left the gate
      if ((state[i].rssiTrigger > triggerThreshold[i]) &&
        (state[i].rssi < (state[i].rssiTrigger - triggerThreshold[i]))) {
        Serial.println("Crossing = False" + i);
        lastPass[i].rssiPeakRaw = state[i].rssiPeakRaw;
        lastPass[i].rssiPeak = state[i].rssiPeak;
        lastPass[i].timeStamp = state[i].rssiPeakRawTimeStamp;
        lastPass[i].lap = lastPass[i].lap + 1;

        state[i].crossing = false;
        state[i].calibrationMode = false;
        state[i].rssiPeakRaw = 0;
        state[i].rssiPeak = 0;
      }
    }
  }

   } 


  
if(workmode>2){

    for (int i=0; i <= 3; i++){
Serial.println(state[i].crossing);
Serial.println(state[i].calibrationMode);
Serial.println(state[i].rssiPeakRaw);
Serial.println(state[i].rssiPeak);
Serial.println(state[i].rssiRaw);
Serial.println(state[i].rssi);
}
}
}





   parseCommands();
}


void connectToWiFi(const char * ssid, const char * pwd){
  Serial.println("Connecting to WiFi network: " + String(ssid));

  // delete old config
  WiFi.disconnect(true);
  //register event handler
  WiFi.onEvent(WiFiEvent);
  
  //Initiate connection
  WiFi.begin(ssid, pwd);

  Serial.println("Waiting for WIFI connection...");
}

//wifi event handler
void WiFiEvent(WiFiEvent_t event){
    switch(event) {
      case SYSTEM_EVENT_STA_GOT_IP:
          //When connected set 
          Serial.print("WiFi connected! IP address: ");
          Serial.println(WiFi.localIP());  
          //initializes the UDP state
          //This initializes the transfer buffer
          udp.begin(WiFi.localIP(),udpPort);
          connected = true;
          break;
      case SYSTEM_EVENT_STA_DISCONNECTED:
          Serial.println("WiFi lost connection");
          connected = false;
          break;
    }
}








void setFrequency(uint16_t frequency, uint16_t rxid) {
    uint16_t fLo = (frequency - 479) / 2;
    uint16_t regN = fLo / 32;
    uint16_t regA = fLo % 32;
    uint16_t synthRegB = (regN << 7) | regA;
    delay(250);
digitalWrite(RECEIVER_PINS[rxid], LOW);
delay(250);
    RCV_CHIP_SELECT=RECEIVER_PINS[rxid];
    RCV_FREQ(f_register[synthRegB]);
delay(250);
digitalWrite(RECEIVER_PINS[rxid], HIGH);
delay(250);
}


void parseCommands() {
    if (Serial.available() > 0) {
        const char command = Serial.read();

        switch (command) {
            // Status
            case '?': {
                Serial.print(F("? "));
                Serial.print(RECEIVER_COUNT, DEC);
                Serial.print(serialSeperator);

                for (uint8_t i = 0; i < RECEIVER_COUNT; i++) {
                   // Serial.print(EepromSettings.frequency[i]);
                    Serial.print(serialSeperator);
                }

               // Serial.print(EepromSettings.rawMode ? 1 : 0, DEC);
                Serial.print(lineEnding);
            } break;

            // Set frequency.
            case 'f': {
                uint8_t receiverIndex = Serial.parseInt();
              uint16_t frequency = Serial.parseInt();

                if (receiverIndex == 0 && frequency == 0)
                    break;

                if (receiverIndex < 0 || receiverIndex >= RECEIVER_COUNT)
                    break;
                  setFrequency(frequency,receiverIndex);
            //    receivers[receiverIndex].setFrequency(frequency);
            //    EepromSettings.frequency[receiverIndex] = frequency;
           //     EepromSettings.save();
            } break;

            // Calibrate minimum RSSI values.
            case 'n': {
                for (uint8_t i = 0; i < RECEIVER_COUNT; i++) {
              //      EepromSettings.rssiMin[i] =
              //          (uint16_t) receivers[i].rssiRaw;
                }

               // EepromSettings.save();
            } break;

            // Calibrate maximum RSSI values.
            case 'x': {
                for (uint8_t i = 0; i < RECEIVER_COUNT; i++) {
                 //   EepromSettings.rssiMax[i] =
                 //       (uint16_t) receivers[i].rssiRaw;
                }

//  EepromSettings.save();
            } break;

            // Enable raw mode.
            case 'b': {
              //  EepromSettings.rawMode = true;
              //  EepromSettings.save();
            } break;

            // Disable raw mode.
            case 's': {
              //  EepromSettings.rawMode = false;
              //  EepromSettings.save();
            } break;
                        case 'm': {
                          uint8_t todo = Serial.parseInt();
          workmode=todo;
        }
        }
        Serial.find('\n');
        Serial.print(F("ok"));
        Serial.print(lineEnding);
    }
}



/*
 * SPI driver based on fs_skyrf_58g-main.c Written by Simon Chambers
 * TVOUT by Myles Metzel
 * Scanner by Johan Hermen
 * Inital 2 Button version by Peter (pete1990)
 * Refactored and GUI reworked by Marko Hoepken
 * Universal version my Marko Hoepken
 * Diversity Receiver Mode and GUI improvements by Shea Ivey
 * OLED Version by Shea Ivey
 * Seperating display concerns by Shea Ivey
 * 
The MIT License (MIT)
Copyright (c) 2015 Marko Hoepken
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
void RCV_FREQ(uint16_t channelData) {
  uint8_t j;
  // Second is the channel data from the lookup table
  // 20 bytes of register data are sent, but the MSB 4 bits are zeros
  // register address = 0x1, write, data0-15=channelData data15-19=0x0
  SERIAL_ENABLE_HIGH();
  SERIAL_ENABLE_LOW();

  // Register 0x1
  SERIAL_SENDBIT1();
  SERIAL_SENDBIT0();
  SERIAL_SENDBIT0();
  SERIAL_SENDBIT0();

  // Write to register
  SERIAL_SENDBIT1();

  // D0-D15
  //   note: loop runs backwards as more efficent on AVR
  for (j = 16; j > 0; j--)
  {
    // Is bit high or low?
    if (channelData & 0x1)
    {
      SERIAL_SENDBIT1();
    }
    else
    {
      SERIAL_SENDBIT0();
    }

    // Shift bits along to check the next one
    channelData >>= 1;
  }

  // Remaining D16-D19
  for (j = 4; j > 0; j--)
    SERIAL_SENDBIT0();

  // Finished clocking data in
  SERIAL_ENABLE_HIGH();
  delayMicroseconds(1);
  //delay(2);

  digitalWrite(RCV_CHIP_SELECT, LOW);
  digitalWrite(RCV_CLK, LOW);
  digitalWrite(RCV_DATA, LOW);
}

void SERIAL_SENDBIT1()
{
  digitalWrite(RCV_CLK, LOW);
  delayMicroseconds(1);

  digitalWrite(RCV_DATA, HIGH);
  delayMicroseconds(1);
  digitalWrite(RCV_CLK, HIGH);
  delayMicroseconds(1);

  digitalWrite(RCV_CLK, LOW);
  delayMicroseconds(1);
}

void SERIAL_SENDBIT0()
{
  digitalWrite(RCV_CLK, LOW);
  delayMicroseconds(1);

  digitalWrite(RCV_DATA, LOW);
  delayMicroseconds(1);
  digitalWrite(RCV_CLK, HIGH);
  delayMicroseconds(1);

  digitalWrite(RCV_CLK, LOW);
  delayMicroseconds(1);
}

void SERIAL_ENABLE_LOW()
{
  delayMicroseconds(1);
  digitalWrite(RCV_CHIP_SELECT, LOW);
  delayMicroseconds(1);
}

void SERIAL_ENABLE_HIGH()
{
  delayMicroseconds(1);
  digitalWrite(RCV_CHIP_SELECT, HIGH);
  delayMicroseconds(1);
}


