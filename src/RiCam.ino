// Project RiCam
//#define TFT
//#include "IO.h"
#if defined TFT
  #include "tft.h"
#endif
//#include "Adafruit_10DOF_IMU.h"
#include "neopixel.h"
#include "Adafruit_TCS34725.h"
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
// ---- Commande Web
int WebCde(String Cde);

String Mois[12] = {"JAN", "FEV", "MAR", "AVR", "MAI", "JUN", "JUI", "AOU", "SEP", "OCT", "NOV", "DEC"};
// Surveillance
double illum_m = 0;
bool alert_illum  = false;
double luminosite,illumination;  // mesure de la luminosite
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_24MS, TCS34725_GAIN_1X);
uint16_t clear, red, green, blue;
double r, g, b, wh;
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
bool Lamp_on = true;
static const byte Lamp_l[13] = { 0, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 255 };
static const byte Lamp_c[8] = { 0, 35, 70, 105, 140, 175, 210, 255 };
#if defined TFT
//--------------------------------------------- TFT  -------
//--                                            -----
// init variable ST7735 : afficheur TFT 1.8" w/SD Card
#define dc      D2 
#define cs      D4 
#define rst     D5 
#define sclk    A3
#define sid     A5
ST7735 tft = ST7735(cs, dc, rst); //l'affichage en mode MISO
#endif

void setup() {
  // Put initialization like pinMode and begin functions here.
  Particle.publish("status", "by e-Coucou 2018");
  Time.zone(+1);
  Wire.setSpeed(CLOCK_SPEED_100KHZ);
  Wire.begin();
  lampe.begin();
  // Start WebCommande
  bool success = Particle.function("Cde",WebCde);
#if defined TFT
  init_tft();
  copyright();

  delay(3000);
#endif
  rainbow(20);
  Lamp_color(0x0, 0xFFFF);
  Particle.variable("luminosite",luminosite);
  Particle.variable("illumination",illumination);
  delay(3000);
}

volatile unsigned long now, start = 0, count =0, iteration=0;
volatile int update = 0;
uint32_t lumiere=0x0;
bool tft_update=true;
char szMessage[30];

void loop() {
  // The core of your code will likely live here.
  now = millis(); count++;
  //---------------------------------------------------------------- SENSORS ------
  //-- Gestion des sensors
  //--
    //-- Toutes les 500ms on récupère les infos de luminosité
    //-- 
    if ((millis() % 1000) >= 500) {
      tcs.getRawData(&red, &green, &blue, &clear);
      wh = clear;
      r = red/wh *256.0;
      g = green/wh *256.0;
      b = blue/wh *256.0;
      illumination =  (-0.32466 * red) + (1.57837 * green) + (-0.73191 * blue); //
      illum_m = (illum_m * 11.0/12.0) + (illumination / 12.0);
      luminosite = illum_m; /// pour debug
      // gestion d'alerte par proximité
//      if ( (abs(illumination) < abs(0.5*illum_m)) && abs(illum_m < 30.0) ) {
      if (abs(illumination <=200.0)) {
         alert_illum = true;
         lumiere = (0xFF - uint8_t(illumination)) << 24;
      } else { alert_illum = false;}
//      Serial.println(String::format("Illumination : %f, luminosite : %f",illumination,luminosite));
    }
/*
  if (count<1000) {
      Lamp_color(0x11FF0000,0xAAAA);
  } else {
      Lamp_color(0x1100FF00,0x5555);
      if (count>2000)
        {count=0;}
  }
*/
    if (count > 10000) {
        count = 0;
        Particle.publish("status", String::format("Illumination : %f, luminosite : %f",illumination,luminosite));
    }
  if (alert_illum & Lamp_on) {
      Lamp_color(lumiere,0xFFFF);
  } else {
      Lamp_color(0x0,0xFFFF); 
  }
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
//-------------------------------------------------------- Gestion de l'écran TFT ----
#if defined TFT
void init_tft() {
//    digitalWrite(POWER,HIGH);
//    delay(30);
    tft.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab
    tft.setRotation(0x03);
}
void copyright() {
    tft.fillScreen(ST7735_BLACK);// 19 19 70 - ST7735_BLACK);
    tft.setCursor(0, 0);
//    tft.setTextColor(tft.Color565(0xAC,0xEE,0xEE),tft.Color565(0x19,0x19,0x70));//,ST7735_BLACK);
    tft.setTextColor(tft.Color565(0xAC,0xEE,0xEE),ST7735_BLACK);
    tft.setTextWrap(true);
    tft.println("Welcome @HOME. \n(c) e-Coucou 2017\n\nDemarrage du SYSTEME\nby rky ...");
    // copyright
    char szMess[50];
    tft.setTextSize(1);
    tft.setTextColor(0xEEEE,ST7735_BLACK);
    tft.setCursor(0, 80);
    sprintf(szMess,"(c)Rky %d.%d %02d:%02d %s",VERSION_MAJ,VERSION_MIN,Time.hour(),Time.minute(),WiFi.ready() ? "Wifi" : " - - ");
    tft.print(szMess);
}
void aff_Heure() {
    char szMess[20];
    int heure = int(Time.hour());
    int minute = int(Time.minute());
    int seconde = int(Time.second());
    sprintf(szMess,"%2d:%s%d",heure,minute>9 ? "":"0",minute);
//    tft.fillRect(0,0,tft.width(),tft.height(),ST7735_BLACK);
//    tft.setTextColor(tft.Color565(0xAF,0xEE,0xEE));
//    tft.fillScreen(ST7735_BLACK);
    tft.setTextColor(ST7735_WHITE,ST7735_BLACK);
    tft.setCursor(8,43);
    tft.setTextSize(3);
    tft.println(szMess);
    tft.setCursor(105,43);
    tft.setTextSize(1);
    tft.println(String::format("%s%d",seconde>9 ? "":"0", seconde));
    tft_update = false;
}
void aff_Date() {
    char szMess[20];
    int jour = int(Time.day());
    int mois = int(Time.month());
    int annee = int(Time.year());
    sprintf(szMess,"%2d/%2d/%4d",jour,mois,annee);
//    tft.fillRect(0,0,tft.width(),tft.height(),ST7735_BLACK);
//    tft.setTextColor(tft.Color565(0xAF,0xEE,0xEE));
//    tft.fillScreen(ST7735_BLACK);
    tft.setTextColor(ST7735_WHITE,ST7735_BLACK);
    tft.setTextSize(1);
    tft.setCursor(137,37);tft.println(String::format("%2d",jour));
    tft.setCursor(133,47);tft.println(Mois[mois-1]);
    tft.setCursor(130,57);tft.println(String::format("%2d",annee));
//    tft.println(szMess);
//    tft_update = false;
}
void aff_Click() {
    char szMess[20];
    sprintf(szMess,"CLICK");
//    tft.fillRect(0,0,tft.width(),tft.height(),ST7735_BLACK);
//    tft.setTextColor(tft.Color565(0xAF,0xEE,0xEE));
    tft.fillScreen(ST7735_RED);
    tft.setTextColor(ST7735_WHITE);
    tft.setCursor(10,50);
    tft.setTextSize(4);
    tft.println(szMess);
    tft_update = false;
}
void aff_Trame() {
  tft.fillScreen(ST7735_BLACK);
  tft.drawFastHLine(0,33,160,ST7735_WHITE);
  tft.drawFastHLine(0,73,160,ST7735_WHITE);
  tft.drawFastVLine(40,0,33,ST7735_WHITE);
  tft.drawFastVLine(80,0,33,ST7735_WHITE);
  tft.drawFastVLine(120,0,120,ST7735_WHITE);
  tft.setTextColor(ST7735_BLUE,ST7735_BLACK);
  tft.setTextSize(1);
  tft.setCursor(1,121);
  tft.println(String::format("%s %d.%d (%s)",AUTEUR,VERSION_MAJ,VERSION_MIN,RELEASE));
  tft.setTextColor(ST7735_WHITE,ST7735_BLACK);
  tft.setCursor(125,1);
//  tft.println(String::format("%5.1f",bytesRead/1024.0));
  tft.setCursor(135,24);
//  tft.println(String::format("%c",isPicture ? 'V' : 'x'));
  aff_Date();
//  getRequest();
//  getBatterie();
//  rainbow(20);
//  Lamp_color(0x0,0xFFFF);
}
#endif
//------------------------------------------------------------------ Web Commande ------
//--                                                                 ------------
int WebCde(String  Cde) {
    
    unsigned char commande=0;
    char szMess[30];

    int index = Cde.indexOf('=');
    char inputStr[64];
    Cde.toCharArray(inputStr,64);
    char *p = strtok(inputStr," ,.-=");
    p = strtok(NULL, " ,.-=");
    int arg[10];
    String arg_s[10];
    int i=0;
    serial_on = Serial.isConnected();
    while (p != NULL)
    {
        arg[i++] = atoi(p);// & 0xFF;
        p = strtok (NULL, " ,.-");
    }
    
    if (Cde.startsWith("coul"))  commande = 0xA0;
    if (Cde.startsWith("on"))  commande = 0xF0;
    switch (commande) {
        case 0xA0: // color,w,g,r,b
            commande  = arg[0] & 0xFF;
//            neo_color = (arg[0] << 24) + (arg[1] << 16) + (arg[2] << 8) + arg[3];
            sprintf(szMess,"Change la couleur wgrb : %X:%X:%X:%X",arg[0],arg[1],arg[2],arg[3]);
            break;
        case 0xF0:// On/Off de la lampe
            Lamp_on = !Lamp_on;
            break;
        default:
            break;
    }
    if (serial_on) {
      Serial.println("WebCde : reception WebCommande -> ");
      Serial.println(szMess);
    }
    return commande;
} // end WebCde
