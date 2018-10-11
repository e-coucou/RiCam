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
// NeoPixel
#define LAMP_PIN D6
#define LAMP_COUNT 16
#define LAMP_TYPE SK6812RGBW
Adafruit_NeoPixel lampe = Adafruit_NeoPixel(LAMP_COUNT, LAMP_PIN, LAMP_TYPE);
void rainbow(uint8_t wait);
uint32_t Wheel(byte WheelPos);
uint16_t w = 0x0000;
uint32_t neo_color,neo_mask,wait_start; // Neo_color : 0xWWGGRRBB
char tour=0;
char Lamp_w = 12;
bool Lamp_on = false;
static const byte Lamp_l[13] = { 0, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 255 };
static const byte Lamp_c[8] = { 0, 35, 70, 105, 140, 175, 210, 255 };

void setup() {
  // Put initialization like pinMode and begin functions here.
  Wire.setSpeed(CLOCK_SPEED_100KHZ);
  Wire.begin();
  lampe.begin();
}

void loop() {
  // The core of your code will likely live here.

}
//-------------------------------------------------------- Gestion des animations LAMPE ----
void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<lampe.numPixels(); i++) {
      lampe.setPixelColor(i, Wheel((i+j) & 255));
    }
    lampe.show();
    delay(wait);
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return lampe.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return lampe.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return lampe.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

int Lamp_color(uint32_t color, uint32_t mask) {
    for (uint16_t i=0; i<lampe.numPixels();i++)
    {
        if (mask & 0x0001 == 1) 
            {   lampe.setPixelColor(i,color);
            } else {
                lampe.setPixelColor(i,0x0);
            }
        mask >>= 1;
    }
    lampe.show();
}