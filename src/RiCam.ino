// Project RiCam
#define TFT
//#include "IO.h"
#if defined TFT
  #include "tft.h"
#endif
//#include "Adafruit_10DOF_IMU.h"
#include "neopixel.h"
//#include "Adafruit_TCS34725.h"
// Version Information
#define NAME "RiCam"
#define AUTEUR "eCoucou"
#define VERSION_MAJ 0
#define VERSION_MIN 1
#define RELEASE "oct. 2018"
#define CREATE "oct. 2018"
/* 
				         -----[    ]-----
				        -|VIN        3V3|-    
				        -|GND        RST|-
				        -|TX        VBAT|-
				        -|RX         GND|-
				        -|WKP         D7|-
				        -|DAC         D6|- NeoPixel Ring
	SID	  |  MOSI -|A5          D5|- RST | TFT
		    |  MISO -|A4          D4|- CS  | TFT
	SCL	  |  SCHK	-|A3          D3|- ALERT interrupt from MAX17043 (optional solder bridge)
   Arducam | SS -|A2          D2|- DC  | TFT
				        -|A1          D1|- SCL |- I2C channel + Gyro
				        -|A0          D0|- SDA |
				          \____________/ 
*/

void setup() {
  // Put initialization like pinMode and begin functions here.

}

void loop() {
  // The core of your code will likely live here.

}