uint64_t chipid;  

//const int RCV_CHIP_SELECT = 5;
//Chip Select Pins 5,17,16,4 

int RCV_CHIP_SELECT = 5;
const int RCV_CLK = 18;
const int RCV_DATA = 19;

uint16_t frequencies[] = {5800, 5740, 0, 0, 0, 0, 0};
uint16_t f_register[] = {0x2984, 0x2906, 0x2a1f, 0, 0, 0, 0};
uint16_t lastValue = 0;
bool increasing = true;
uint16_t tuningFrequency = 0;
uint16_t tunedFrequency = 0;
int val0=0;
int val1=0;
int val2=0;
int val3=0;


void setup() {

  delay(500);
  pinMode(RCV_DATA, OUTPUT);
  pinMode(RCV_CLK, OUTPUT);
  //pinMode(RCV_CHIP_SELECT, OUTPUT);
  pinMode(5, OUTPUT);
 	pinMode(17, OUTPUT);
  pinMode(16, OUTPUT);
  pinMode(4, OUTPUT);
  
	Serial.begin(250000);


   chipid=ESP.getEfuseMac();//The chip ID is essentially its MAC address(length: 6 bytes).
  Serial.printf("ESP32 Chip ID = %04X",(uint16_t)(chipid>>32));//print High 2 bytes
  Serial.printf("%08X\n",(uint32_t)chipid);//print Low 4bytes.
delay(500);

RCV_CHIP_SELECT=5;
RCV_FREQ(f_register[2]);
digitalWrite(5, HIGH);
delay(500);
RCV_CHIP_SELECT=17;
RCV_FREQ(f_register[0]);
digitalWrite(17, HIGH);
delay(500);
RCV_CHIP_SELECT=16;
RCV_FREQ(f_register[2]);
digitalWrite(16, HIGH);
delay(500);
RCV_CHIP_SELECT=4;
RCV_FREQ(f_register[0]);
digitalWrite(4, HIGH);





}

void loop() {
val0 = analogRead(36);    // read the input pin
val1 = analogRead(39);    // read the input pin
val2 = analogRead(34);    // read the input pin
val3 = analogRead(35);    // read the input pin

Serial.print(val0);
Serial.print(" ");
Serial.print(val1);
Serial.print(" ");
Serial.print(val2);
Serial.print(" ");
Serial.print(val3);
Serial.println(";");

  

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


