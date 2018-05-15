//ESP32 LAPTIMER 4 Workmodes....
//
// ADD WEBSERVER ASYNC
//
// Init with Serial-Monitor manualy type in line by line
// m1 switch to tracking mode
// m6 to active crossing
// m4 to calibrate the nodes
// power on the quad to calibrate the high-level
// m3 to diable calibration mode
// m1 to resume tracking

#include <WiFi.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include <WebServer.h>
#include "FS.h"
#include "SPIFFS.h"

const char * networkName = "";
const char * networkPswd = "";


const char * udpAddress = "10.0.0.55";
const int udpPort = 1236;


//Are we currently connected?
boolean connected = false;

//The udp library class
WiFiUDP udp;
WebServer server(80);

uint64_t chipid;

//const int RCV_CHIP_SELECT = 5;
//Chip Select Pins 5,17,16,4

const int RECEIVER_PINS[] = {5, 17, 16, 4};
const int ADC_PINS[] = {36, 39, 34, 35};
int triggerThreshold[3];
uint32_t lastLoopTimeStamp[3];
#define RECEIVER_COUNT 4
int RCV_CHIP_SELECT = 5;
const int RCV_CLK = 18;
const int RCV_DATA = 19;


//uint16_t frequencies[] = {5800, 5740, 0, 0, 0, 0, 0};
uint16_t f_register[] = {0x2984, 0x2906, 0x2a1f, 0x281D, 0, 0, 0};

uint16_t lastValue = 0;
bool increasing = true;
uint16_t tuningFrequency = 0;
uint16_t tunedFrequency = 0;
int val0 = 0;
int val1 = 0;
int val2 = 0;
int val3 = 0;

const char serialSeperator = ',';

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
  uint32_t volatile lastLoopTimeStamp = 0;

} state[3];

struct {
  uint16_t volatile rssiPeakRaw;
  uint16_t volatile rssiPeak;
  uint32_t volatile timeStamp;
  uint8_t volatile lap;
} lastPass[3];


struct {
  uint16_t volatile minimum;
  uint16_t volatile maximum;
  uint16_t volatile middle;

} rssifilter[3];


int workmode = 1;





void setup() {

  Serial.begin(250000);
 
  connectToWiFi(networkName, networkPswd);

  // Initialize lastPass defaults
  for (int i = 0; i <= 3; i++) {
    settings[i].filterRatioFloat = settings[i].filterRatio / 1000.0f;
    state[i].rssi = 0;
    state[i].rssiTrigger = 0;
    lastPass[i].rssiPeakRaw = 0;
    lastPass[i].rssiPeak = 0;
    lastPass[i].lap = 0;
    lastPass[i].timeStamp = 0;

    rssifilter[i].minimum=0;
    rssifilter[i].maximum=1024;
    

  }


settings[0].vtxFreq=5808;
settings[1].vtxFreq=5675;
settings[2].vtxFreq=5880;
settings[3].vtxFreq=5605;




  delay(500);
  pinMode(RCV_DATA, OUTPUT);
  pinMode(RCV_CLK, OUTPUT);
  //pinMode(RCV_CHIP_SELECT, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(17, OUTPUT);
  pinMode(16, OUTPUT);
  pinMode(4, OUTPUT);




  chipid = ESP.getEfuseMac(); //The chip ID is essentially its MAC address(length: 6 bytes).
  Serial.printf("ESP32 Chip ID = %04X", (uint16_t)(chipid >> 32)); //print High 2 bytes
  Serial.printf("%08X\n", (uint32_t)chipid); //print Low 4bytes.
  delay(500);


setFrequency(5658,0);
setFrequency(5658,1);
setFrequency(5658,2);
setFrequency(5658,3);


   //SET Frequency OLD
   /*
  RCV_CHIP_SELECT = 5;
  RCV_FREQ(f_register[3]); //LETZTE?
  delay(500);
  digitalWrite(5, HIGH);
  delay(500);
  RCV_CHIP_SELECT = 17;
  RCV_FREQ(f_register[3]); //Vorletzte
  delay(500);
  digitalWrite(17, HIGH);
  delay(500);
  RCV_CHIP_SELECT = 16;
  RCV_FREQ(f_register[3]);
  delay(500);
  digitalWrite(16, HIGH);
  delay(500);
  RCV_CHIP_SELECT = 4;
  RCV_FREQ(f_register[3]);
  delay(500);
  digitalWrite(4, HIGH);
  delay(500);
*/
  workmode =1;

 SPIFFS.begin();
  //SERVER INIT
  //called when the url is not defined here
  //use it to load content from SPIFFS
  server.onNotFound([](){
  if(!handleFileRead(server.uri()))
    server.send(404, "text/plain", "FileNotFound");
  });
  server.begin();

  
}
int b = 0;
int counter=0;
void loop() {
 server.handleClient();
     


 
  if (workmode == 0) {
   
    val0 = analogRead(36);   // read the input pin
    val1 = analogRead(39);    // read the input pin
    val2 = analogRead(34);    // read the input pin
    val3 = analogRead(35);    // read the input pin
 /*
    val0 = rssi(0);    // read the input pin
    val1 =  rssi(1);    // read the input pin
    val2 =  rssi(2);   // read the input pin
    val3 =  rssi(3);    // read the input pin
*/

    //Serial.print(F("r "));


    Serial.print(micros());

    Serial.print(serialSeperator);

    Serial.print(val0);

    Serial.print(serialSeperator);

    Serial.print(val1);

    Serial.print(serialSeperator);
    Serial.print(val2);

    Serial.print(serialSeperator);

    Serial.print(val3);

    Serial.print("\r\n");
    b = b + 1;
    if (connected) {
      //Send a packet


      udp.beginPacket(udpAddress, udpPort);

      //udp.printf("Seconds since boot: %u", millis()/1000);
      udp.printf("%u %u %u %u", val0, val1, val2, val3);

      udp.endPacket();
      delay(25);
    }


  }

  for (int i = 0; i <= 3; i++) {
    if (workmode == 3) {

      state[i].calibrationMode = false;

    }
    if (workmode == 4) {
      state[i].calibrationMode = true;
    }
    if (workmode == 5) {
      state[i].rssiTrigger = 0;
    }
    if (workmode == 6) {
      state[i].rssiTrigger = 1;
    }

    if (workmode == 7) {

    }
  }
  if (workmode == 1 or workmode == 3 or workmode == 4) {

counter=counter+1;

    
    for (int i = 0; i <= 3; i++) {




      // Calculate the time it takes to run the main loop
      lastLoopTimeStamp[i] = state[i].lastLoopTimeStamp;
      state[i].lastLoopTimeStamp = micros();
      state[i].loopTime = state[i].lastLoopTimeStamp - lastLoopTimeStamp[i];


      state[i].rssiRaw = analogRead(ADC_PINS[i]);
      state[i].rssiSmoothed = (settings[i].filterRatioFloat * (float)state[i].rssiRaw) + ((1.0f - settings[i].filterRatioFloat) * state[i].rssiSmoothed);
      //state.rssiSmoothed = state.rssiRaw;
      state[i].rssi = (int)state[i].rssiSmoothed;

      if (state[i].rssiTrigger > 0) {
        if (!state[i].crossing && state[i].rssi > state[i].rssiTrigger) {
          state[i].crossing = true; // Quad is going through the gate
          Serial.print("Crossing = True Node: ");
          Serial.println(i);

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
          //  Serial.println("in calibration" + i);
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
            Serial.print("Lap: ");
            Serial.println(lastPass[i].lap);
            Serial.println(lastPass[i].timeStamp);
            state[i].crossing = false;
            state[i].calibrationMode = false;
            state[i].rssiPeakRaw = 0;
            state[i].rssiPeak = 0;


            //SEND TO UDP SERVICE GATEWAY
            if (connected) {
              //Send a packet


              udp.beginPacket(udpAddress, udpPort);
              //{"node":0,"'frequency": 5808, "timestamp": 11111}
              //udp.printf("Seconds since boot: %u", millis()/1000);   {'node': 1, 'frequency': 5808, 'timestamp': 111554455}
              //udp.printf("%u %u %u %u", val0, val1, val2,val3);

              //udp.printf("{'node': %u, 'frequency': 5808, 'timestamp': 111554455}", i);
              //udp.printf("%u %u", i,lastPass[i].timeStamp);
              udp.printf("pass_record|{\"node\":%u,\"frequency\": %u, \"timestamp\": %u}", i, settings[i].vtxFreq,lastPass[i].timeStamp);
              udp.endPacket();
        
            }


          }
        }
      }

    }

/*
if (connected and counter>=100000) {
        
              //Send a packet


             udp.beginPacket(udpAddress, udpPort);
              //{"node":0,"'frequency": 5808, "timestamp": 11111}
              //udp.printf("Seconds since boot: %u", millis()/1000);   {'node': 1, 'frequency': 5808, 'timestamp': 111554455}
              //udp.printf("%u %u %u %u", val0, val1, val2,val3);

              //udp.printf("{'node': %u, 'frequency': 5808, 'timestamp': 111554455}", i);
              //udp.printf("%u %u", i,lastPass[i].timeStamp);
              //mysocket.emit('heartbeat', {'current_rssi': [900,400,400,900]});
              //udp.printf("heartbeat|{\"current_rssi\": [200,200,200,200]}");

              //udp.printf("heartbeat|{\"current_rssi\": [%u,%u,%u,%u]}", );
              udp.endPacket();
          counter=0;
            }

*/
    if (workmode == 2) {

      for (int i = 0; i <= 3; i++) {
        Serial.println(state[i].crossing);
        Serial.println(state[i].calibrationMode);
        Serial.println(state[i].rssiPeakRaw);
        Serial.println(state[i].rssiPeak);
        Serial.println(state[i].rssiRaw);
        Serial.println(state[i].rssi);
      }
    }
  }
  if (workmode == 9) {
    Serial.println("%HRT  0 0.000 1");
    Serial.println("#RAC");
    Serial.println("@RAC  4 0.000");
    Serial.println("%LAP  4 5.548 1 0 5.548 461 311 211");




    delay(1000);

  }


  parseCommands();
}

String getContentType(String filename){
  if(server.hasArg("download")) return "application/octet-stream";
  else if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path){
 
  Serial.println("handleFileRead: " + path);
  if(path.endsWith("/")) path += "mobile.html";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
    if(SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }

  
  //server.send(200, "text/plain", "hello from esp32!");
  return true;
}

void connectToWiFi(const char * ssid, const char * pwd) {
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
void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      //When connected set
      Serial.print("WiFi connected! IP address: ");
      Serial.println(WiFi.localIP());
      //initializes the UDP state
      //This initializes the transfer buffer
      udp.begin(WiFi.localIP(), udpPort);
      connected = true;
      //server.begin();
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi lost connection");
      connected = false;
      break;
  }
}


// Calculate rx5808 register hex value for given frequency in MHz
uint16_t freqMhzToRegVal(uint16_t freqInMhz) {
  uint16_t tf, N, A;
  tf = (freqInMhz - 479) / 2;
  N = tf / 32;
  A = tf % 32;
  return (N<<7) + A;
}



void setFrequency(uint16_t frequency, uint16_t rxid) {
 /*
  uint16_t fLo = (frequency - 479) / 2;
  uint16_t regN = fLo / 32;
  uint16_t regA = fLo % 32;
  uint16_t synthRegB = (regN << 7) | regA;
  */
  delay(250);
  digitalWrite(RECEIVER_PINS[rxid], LOW);

  RCV_CHIP_SELECT = RECEIVER_PINS[rxid];

  
  //RCV_FREQ(f_register[synthRegB,HEX]);
 
  RCV_FREQ(freqMhzToRegVal(frequency));

  digitalWrite(RECEIVER_PINS[rxid], HIGH);
  settings[rxid].vtxFreq = frequency;
  delay(250);
}
/*
long mapA(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

uint16_t rssi(int rxid){
 uint16_t vali = analogRead(ADC_PINS[rxid]);
  
  
  vali = mapA(vali, 2, rssifilter[rxid].minimum, 2, rssifilter[rxid].maximum);
  
  return vali;
}*/


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
          Serial.print("\r\n");
        } break;

      // Set frequency.
      case 'f': {
          uint8_t receiverIndex = Serial.parseInt();
          uint16_t frequency = Serial.parseInt();

          if (receiverIndex == 0 && frequency == 0)
            break;

          if (receiverIndex < 0 || receiverIndex >= RECEIVER_COUNT)
            break;
          setFrequency(frequency, receiverIndex);
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
      case 'r': {
           for (int i = 0; i <= 3; i++) {
            state[i].calibrationMode=true;
            state[i].rssiTrigger = state[i].rssi - settings[i].calibrationOffset;
            lastPass[i].rssiPeakRaw = 0;
            lastPass[i].rssiPeak = 0;
            state[i].rssiPeakRaw = 0;
            state[i].rssiPeakRawTimeStamp = 0;
            //   EepromSettings.rssiMax[i] =
            //       (uint16_t) receivers[i].rssiRaw;
          }

          //  EepromSettings.save();
        } break;

      // Enable raw mode.
      case 'b': {
        Serial.println("Power off Quad");
        for (int i = 0; i <= 3; i++) {
        
         rssifilter[i].minimum=analogRead(ADC_PINS[i]);
         
         }
         delay(2000);
         Serial.println("Power on Quad");
           for (int i = 0; i <= 3; i++) {
         
         rssifilter[i].maximum=analogRead(ADC_PINS[i]);
         
         
          
        }
        delay(2000);
Serial.println("Done");
        
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
          workmode = todo;
        }
    }
    Serial.find('\n');
    Serial.print(F("ok"));
    Serial.print("\r\n");
  }
}



/*
   SPI driver based on fs_skyrf_58g-main.c Written by Simon Chambers
   TVOUT by Myles Metzel
   Scanner by Johan Hermen
   Inital 2 Button version by Peter (pete1990)
   Refactored and GUI reworked by Marko Hoepken
   Universal version my Marko Hoepken
   Diversity Receiver Mode and GUI improvements by Shea Ivey
   OLED Version by Shea Ivey
   Seperating display concerns by Shea Ivey

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

/***************************************************
  Send and receive data from Local Page
***************************************************
void WiFiLocalWebPageCtrl(uint16_t filename)
{
  WiFiClient client = server.available();   // listen for incoming clients
  //client = server.available();
  if (client) {                             // if you get a client,
    Serial.println("New Client.");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            //WiFiLocalWebPageCtrl();
            client.print("Temperature now is: ");
            //client.print(localTemp);
            client.print("  oC<br>");
            client.print(settings[0].vtxFreq);
             client.print("<br>"); 
                         client.print(settings[1].vtxFreq);
             client.print("<br>");     
                         client.print(settings[2].vtxFreq);
             client.print("<br>");     
                         client.print(settings[3].vtxFreq);
             client.print("<br>");               
            //client.print(localHum);
            client.print(" % <br>");
            client.print("<br>");
            client.print("Analog Data:     ");
            //client.print(analog_value);
            client.print("<br>");
            client.print("<br>");

            client.print("Click <a href=\"/H\">here</a> to turn the LED on.<br>");
            client.print("Click <a href=\"/L\">here</a> to turn the LED off.<br>");

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /H" or "GET /L":
        if (currentLine.endsWith("GET /H")) {
          // digitalWrite(LED_PIN, HIGH);               // GET /H turns the LED on
        }
        if (currentLine.endsWith("GET /L")) {
          //   digitalWrite(LED_PIN, LOW);                // GET /L turns the LED off
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }
}
*/
