Test for ESP32 as Laptimer!

Please use this as Firmware on the ESP32
https://github.com/bilson/wizardtracker-hardware/

Check this lines in the config file
#define ESP32

and the Pinout
    #define RECEIVER_PIN_SPI_CLK 18
    #define RECEIVER_PIN_SPI_DATA 19
  
    const int RECEIVER_PINS_SPI_SS[] = {5, 17, 16, 4};
    const int RECEIVER_PINS_RSSI[] = {36, 39, 34, 35};
    
    
 Software:
 
 Install node-red... you can choose linux, windows, what you want... 
 
 import my flow 
 https://raw.githubusercontent.com/realhuno/esp32_laptimer/master/esp32_laptimer/node-red.txt
 
 
 connect to node-red 127.0.0.1:1880
you see
https://raw.githubusercontent.com/realhuno/esp32_laptimer/master/esp32_laptimer/node-red.PNG
