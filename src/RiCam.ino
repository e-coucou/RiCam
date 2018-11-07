// Project RiCam
#define TFT
//#include "IO.h"
#if defined TFT
  #include "tft.h"
#endif
#include "Adafruit_10DOF_IMU.h"
#include "neopixel.h"
#include "Adafruit_TCS34725.h"
#include "HttpClient.h"
#include "clickButton.h"
#include "menu.h"
// Version Information
#define NAME "RiCam"
#define AUTEUR "eCoucou"
#define VERSION_MAJ 0
#define VERSION_MIN 4
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
	     SID  |  MOSI   -|A5          D5|- RST | TFT
		      |  MISO   -|A4          D4|- CS  | TFT
	 SCL	  |  SCHK	-|A3          D3|- ALERT interrupt from MAX17043 (optional solder bridge)
           Arducam | SS -|A2          D2|- DC  | TFT
				        -|A1          D1|- SCL |- I2C channel + Gyro
				        -|A0          D0|- SDA |
				          \____________/ 
*/
void aff_cls(uint16_t couleur=ST7735_BLACK);
void aff_Click(String szMess, uint16_t couleur = ST7735_RED);
void aff_Compteur(float val, char *mesure, float r = 0.0);
// ---- Commande Web
int WebCde(String Cde);
bool serial_on, b_Message = false;
uint8_t mode = MODE_CYCLE; // 0=cycle, 1=lampe, 2= menu

String Mois[12] = {"JAN", "FEV", "MAR", "AVR", "MAI", "JUN", "JUI", "AOU", "SEP", "OCT", "NOV", "DEC"};
struct stParam {
  uint8_t version, resolution;
  unsigned long timeout;
};
stParam Param = {1,0,60000};

struct stAccuWeather {
  float Temperature,Vitesse,Ressentie,Pression;
  String ciel,Sens,PressionTrend;
  int Direction,Humidite;
  bool jour, data;
};
stAccuWeather Meteo;
uint8_t meteoS = 4;  // 4=Lyon 6
// Surveillance
double illum_m = 0;
bool alert_illum  = false;
double luminosite,illumination;  // mesure de la luminosite
double pression, temperature;
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
uint32_t Lamp_couleur=0x0010FF00,Lamp_mask=0xFFFF,wait_start; // Neo_color : 0xWWGGRRBB
char tour=0;
char Lamp_w = 12;
bool Lamp_on = false;
bool Lamp_auto = true;
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
// -------------------------------------------------------gestion des boutons --------
void click(void); // la function d'interruption
int clk = D3; // bouton
ClickButton down(clk,LOW,CLICKBTN_PULLUP);
int Button = 0;
//--------------------------------------------- Capteurs  -------
//--                                            -------
Adafruit_BMP085_Unified       bmp   = Adafruit_BMP085_Unified(18001);
Adafruit_LSM303_Accel_Unified accel = Adafruit_LSM303_Accel_Unified(30301);
Adafruit_LSM303_Mag_Unified   mag   = Adafruit_LSM303_Mag_Unified(30302);
Adafruit_L3GD20_Unified       gyro  = Adafruit_L3GD20_Unified(20);
sensor_t sensor;
//--
Timer tm_cloud(20000,Cloud);
bool tm_b_cloud = false;
int tm_cloud_rot = 0x00;
Timer tm_cycle(10000,Cycle);
bool tm_b_cycle = false;
int tm_cycle_rot = 0x00;
Timer tm_aff(150000,Aff);
bool tm_b_aff = true;
//--------------------- timer CLOUD : every 20 sec. ---
void Cloud() {
    tm_b_cloud = true;
}
void Aff() {
    tm_b_aff = true;
}
void Cycle() {
    tm_b_cycle = true;
}
HttpClient http;  
http_header_t headers[] = {  
    { "Content-Type", "application/json" },  
    { "Accept" , "application/json" },
    { NULL, NULL }   
 };  
http_request_t request;  
http_response_t response;  
//---------------------------------------------------------------------------- SETUP ---
void setup() {
  // Put initialization like pinMode and begin functions here.
  Particle.publish("status", "by e-Coucou 2018");
  Time.zone(+1); // +1 heure d'hiver vs +2 ete
  Particle.variable("luminosite",luminosite);
  Particle.variable("illumination",illumination);
  Particle.variable("temperature",temperature);
  Particle.variable("pression",pression);
  Wire.setSpeed(CLOCK_SPEED_100KHZ);
  Wire.begin();
  lampe.begin();
  // Start WebCommande
  bool success = Particle.function("Cde",WebCde);
  // Setup button timers (all in milliseconds / ms)
  // (These are default if not set, but changeable for convenience)
  down.debounceTime   = 20;   // Debounce timer in ms
  down.multiclickTime = 250;  // Time limit for multi clicks
  down.longClickTime  = 1000; // time until "held-down clicks" register
  rainbow(20);
  Lamp_color(0x0, 0xFFFF);
  delay(3000);
  SPI.begin();
  #if defined TFT
    init_tft();
    copyright();
    delay(3000);
  #endif
  tm_cloud.start();
  tm_aff.start();
  tm_cycle.start();
  // les capteurs ...
    bmp.begin();
    gyro.begin();
    accel.begin();
    mag.enableAutoRange(true);
    if (!mag.begin()) {
        tft.println("mag not started");
    }
  aff_Mosaic();
  delay(3000);
}
//---------------------------------------
// variables LOOP
volatile unsigned long now, start = 0, count =0, iteration=0;
volatile int update = 0;
uint32_t lumiere=0x0;
bool tft_update=true;
char szMessage[144];
sensors_event_t event;
uint8_t regint = 0x00; // registre d'interruptions
//-------------------------------------------------------------------------------- LOOP ---
void loop() {
  // The core of your code will likely live here.
  now = millis(); count++;
  //---------------------------------------------------------------- SENSORS ------
  //-- Gestion des sensors
  //--
    //-- Toutes les 500ms on récupère les infos de luminosité
    //-- 
    if ((millis() % 500) >= 400) {
      tcs.getRawData(&red, &green, &blue, &clear);
      wh = clear;
      r = red/wh *256.0;
      g = green/wh *256.0;
      b = blue/wh *256.0;
      illumination =  (-0.32466 * red) + (1.57837 * green) + (-0.73191 * blue); //
      illum_m = (illum_m * 11.0/12.0) + (illumination / 12.0);
      luminosite = illum_m; /// pour debug moyenne glissante de 12
      lumiere = 0x0;
      if (abs(illumination <=15.0)) {
         alert_illum = true;
         lumiere = uint8_t((15.0 - illumination)/15.0*255.0);
      } else { alert_illum = false;}
//      getAccelgyro();
      getCompass();
    }
    if ((millis() % 1000) >= 900) {
        aff_Seconde();
    }
  // Gestion de l'allumage de la Lampe  
  if ( Lamp_on) {
      if (Lamp_auto & alert_illum) {
        Lamp_color(lumiere << 24,0xFFFF);
      } else {
        Lamp_color(Lamp_couleur,Lamp_mask);
      }
   } else {
      Lamp_color(0x0,0xFFFF); 
  }
  if (tm_b_aff) {
      aff_Code("...");
      getRequest();
      tm_b_aff = false;
  }
  if (b_Message) {
      aff_Click(szMessage);
  }
  //----------------------------------------------------------------- le Bouton -------------
    down.Update();
    if (down.clicks !=0) {
        b_Message = false;
        Button = down.clicks;
        tm_cycle.reset(); b_Message=false;
        switch (Button) {
            case 1 : // 1 click : on fait +1 sur luminosité et sous menu
                bouton1();
                break;
            case 2 : // 2 click : on passe au sous menu suivant. (mode Lamp_on on change de couleur)
                bouton2();
                break;
            case 3 : // 3 click : on switch  sur mode Lampe
                bouton3();
                break;
            case -1 : // 1 long : on switch sur le mode menu
                aff_cls();
                if (mode == MODE_MENU) {
                    mode = MODE_CYCLE;
                } else {
                    mode = MODE_MENU;
                    menuC = 5; menuS = 1; menu = 1;
                }
                break;
        }
        Button = 0x00;
        down.clicks = 0;
        if (mode == MODE_MENU) {
          switch (menu) {
            case 0x50: // Exit menu, retour au mode Cycle
                mode = MODE_CYCLE;
                tm_b_cycle = true;
                break;
            case 0x0130: case 0x330: case 0x0230: 
                menuC = 5; menuS = 1; menu = 1;
            case 0 : menu=1; menuS = 1;
            case 1 : // Capteurs
                aff_cls();
            case 2 : // Gyro 
            case 3 : // Meteo
            case 4 : // Lampe
            case 5 : // Exit
                affMenu(menuC,&menu_principal[0]);
                break;
            case 0x10: case 0x14 : menu=0x10;
                menuC = 4; aff_Compteur(temperature,"Temp..."); break;
            case 0x11: aff_Compteur(pression,"Pression",(pression-913.25)/2.0); break;
            case 0x12: aff_Compteur(illumination,"Luminosite"); break;
            case 0x13: aff_Click("Retour (..)",ST7735_BLUE); break;
            case 0x20: case 0x24: menu=0x20;
                menuC = 4;aff_cls(); tft.setCursor(10,20); tft.println(String::format("%5.0f",f_gx));break;
            case 0x21: aff_cls();tft.setCursor(10,20);tft.println(String::format("%5.0f",f_ax));break;
            case 0x22: aff_cls();tft.setCursor(10,20);tft.println(String::format("%5.0f",event.magnetic.x));break;
            case 0x23: aff_Click("Retour (..)",ST7735_BLUE); break;
            case 0x3000: case 0x3010 : case 0x3020 : case 0x3030: case 0x3040 : case 0x3050 : case 0x3060 : case 0x3070:
                meteoS = uint8_t(menuO & 0x0F);
            case 0x30: case 0x34 : menu=0x30;
                menuC = 4;menuS=1;aff_cls();
            case 0x31:
            case 0x32:
            case 0x33:
                affMenu(menuC,&menu_meteo[0]);
                break;
            case 0x300: case 0x308 : menu=0x300;
                menuC = 8; menuS= 1; aff_cls();
            case 0x301 : case 0x302 : case 0x303: case 0x304 : case 0x305 : case 0x306 : case 0x307:
                affMenu(menuC,&menu_lieux[0]);
                break;
            case 0x310 : getRequest();menu=0x30;menuS=1;break;
            default : //erreur on revient ancien menu car menu non implémenté
                menu = menuO;
                break;
          }
          aff_Status(0,80,String::format("[%X] %d",menu,meteoS));
        }
        aff_Entete();
    }
  //---------------------------------------------------------------- CYCLE -----
  if (tm_b_cycle & mode==MODE_CYCLE) {
      tm_b_cycle = false;
      tft.fillScreen(ST7735_BLACK);
      aff_Entete();
      switch(tm_cloud_rot++) {
          case 0x00:
            aff_Status(20,20,"RiCam");
            aff_Status(35,20," .   : defilement");
            aff_Status(45,20," ..  : valide");
            aff_Status(55,20," ... : lampe");
            aff_Status(65,20," --- : menu");
            aff_Status(88,20,"by eCoucou 2018");
            break;
          case 0x01 :
            aff_Date();break;
          case 0x02 :
            if (Meteo.data) {
                aff_Meteo(true);
            } else {
                aff_Status(50,10,"No data from Web !");
            }
            break;
          case 0x03 :
            aff_Capteur();
            break;
          case 0x04 :
            aff_Mosaic();
            break;
          case 0x05 :
            aff_Mag();
            break;
          case 0x06 :
            aff_Accel();
            break;
          case 0x07 :
            aff_Gyro();
            break;
          default :
            aff_Heure();tm_cloud_rot=0; break;
      }
  }
   //-------------------------------------------------------------- CLOUD -------
    //-- Cloud / WebHook
    //--
    //-- le timer Cloud() monte le booléen tm_b_cloud pour la lecteur des sensors
    //-- les infos sont transférés sur le cloud 1 par 1 pour ne pas saturer la funcion Publish (évite mode burst)
    //-- webhook : toutes les variables commencent par Rky_
    //--
    if (tm_b_cloud) {
        aff_Code("(.)");
        bmp.getEvent(&event);
        if (event.pressure)
        {
            float tempe;
            /* Display ambient temperature in C */
            bmp.getTemperature(&tempe);
            temperature = double(tempe);
            pression = double(event.pressure);
        }
        switch(tm_cloud_rot++) {
            case 0x00 :
                Particle.publish("Rky_I",String(illumination),60,PUBLIC);
                break;
            case 0x01 :
                //Particle.publish("Rky_r",String(red),60,PUBLIC);
                break;
            case 0x02 :
                //Particle.publish("Rky_g",String(green),60,PUBLIC);
                break;
            case 0x03 :
                //Particle.publish("Rky_b",String(blue),60,PUBLIC);
                break;
            case 0x04 :
                //Particle.publish("Rky_N",String(noise),60,PUBLIC);
                break;
            case 0x05 :
                Particle.publish("Rky_Lu",String(luminosite),60,PUBLIC);
                break;
            case 0x06 :
                //Particle.publish("Rky_Pi",String(pitch),60,PUBLIC);
                break;
            case 0x07 :
                //Particle.publish("Rky_Ro",String(roll),60,PUBLIC);
                break;
            case 0x08 :
                Particle.publish("Rky_T",String(temperature),60,PUBLIC);
                break;
            case 0x09 :
                //Particle.publish("Rky_H",String(bme_h),60,PUBLIC);
                break;
            case 0x0A :
                Particle.publish("Rky_P",String(pression),60,PUBLIC);
                break;
            case 0x0B :
                Particle.publish("Rky_Lm",String(lumiere),60,PUBLIC);
                break;
        }
        tm_cloud_rot %= 12; // car 12 sensors publiés dans le cloud
        tm_b_cloud = false;
    } // end Cloud
} //------------------------------------- End LOOP --
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
// tft 128x160
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
    tft.println("Welcome @RiCam. \n(c) e-Coucou 2018\n\nDemarrage du SYSTEME\nby rky ...");
    // copyright
    char szMess[50];
    tft.setTextSize(1);
    tft.setTextColor(0xEEEE,ST7735_BLACK);
    tft.setCursor(0, 80);
    sprintf(szMess,"(c)Rky - RiCam %d.%d %02d:%02d\n%s",VERSION_MAJ,VERSION_MIN,Time.hour(),Time.minute(),WiFi.ready() ? "Wifi" : " - - ");
    tft.print(szMess);
}
void aff_Heure() { //128x160
    int heure = int(Time.hour());
    int minute = int(Time.minute());
    int seconde = int(Time.second());
    tft.setTextColor(tft.Color565(0xA0,0xA0,0xFF),ST7735_BLACK);
    tft.setTextSize(3);
    tft.setCursor(62,42);tft.println(String::format("%s%d",heure>9 ? "":"0",heure));
    tft.setCursor(62,67);tft.println(String::format("%s%d",minute>9 ? "":"0",minute));
    tft.setCursor(145,1);
    tft.setTextSize(1);
    tft.println(String::format("%s%d",seconde>9 ? "":"0", seconde));
    tft_update = false;
}
void aff_Seconde() { //128x160
    int seconde = int(Time.second());
    tft.setTextColor(tft.Color565(0xA0,0xA0,0xA0),ST7735_BLACK);
    tft.setCursor(145,1);
    tft.setTextSize(1);
    tft.println(String::format("%s%d",seconde>9 ? "":"0", seconde));
    if (seconde==0 & tm_cycle_rot>3) {
        aff_Heure();
    }
    tft_update = false;
}
void aff_Status(uint8_t ligne, uint8_t colonne,String szMess) { //128x160
    tft.setTextColor(tft.Color565(0xC0,0xC0,0xC0),ST7735_BLACK);
    tft.setCursor(colonne,ligne);
    tft.setTextSize(1);
    tft.println(szMess);
    tft_update = false;
}
void aff_Entete() { //128x160
    tft.setTextColor(tft.Color565(0xFF,0x10,0x00),ST7735_BLACK);
    tft.setCursor(1,1);
    tft.setTextSize(1);
    tft.println(String::format("RiCam %d.%d [%d]",VERSION_MAJ,VERSION_MIN,mode));
    tft.drawFastHLine(0,127,75,ST7735_BLUE);
    tft.drawFastHLine(75,127,10,ST7735_WHITE);
    tft.drawFastHLine(85,127,75,ST7735_RED);
    tft_update = false;
}
void aff_Date() {
    int jour = int(Time.day());
    int mois = int(Time.month());
    int annee = int(Time.year());
    tft.setTextColor(tft.Color565(0xA0,0xA0,0xF0),ST7735_BLACK);
    tft.setTextSize(3);
    tft.setCursor(51,29);tft.println(String::format("%s%d ",jour>9 ? "":"0",jour));
    tft.setCursor(51,52);tft.println(Mois[mois-1]);
    tft.setCursor(51,75);tft.println(String::format(" %2d",annee-2000));
    tft_update = false;
}
void aff_Click(String szMess,uint16_t couleur) {
    tft.fillRect(0,12,tft.width(),tft.height()-15,couleur);
    //    tft.setTextColor(tft.Color565(0xAF,0xEE,0xEE));
    //tft.fillScreen(couleur);
    int l = strlen(szMess);
    tft.setTextColor(ST7735_WHITE);
    tft.setCursor(80-(l*5),50);
    tft.setTextSize(2);
    tft.println(szMess);
    tft_update = false;
}
void aff_cls(uint16_t couleur) { // Clear Screen Black)
    tft.fillScreen(couleur);// 19 19 70 - ST7735_BLACK);
}
void aff_Meteo(bool up) { // à perfectionner ...
    int l;
    tft.setTextColor(tft.Color565(0xC0,0xC0,0xFF),ST7735_BLUE);
    tft.setTextSize(2);
    l = strlen(menu_lieux[menuS]);
    tft.fillRect(1,20,tft.width()-2,19,ST7735_BLUE);
    tft.setCursor(80-(l*6),22);tft.println(menu_lieux[meteoS]);
    tft.setTextSize(1);
    tft.setCursor(140,22);tft.println(String::format("(%c)",Meteo.jour ? 'J':'N'));
    tft.setTextColor(tft.Color565(0x80,0x80,0xC0),ST7735_BLACK);
    l=strlen(lieux[meteoS]);
    tft.setCursor(160-(l*6),118);tft.println(lieux[meteoS]);
    tft.setTextSize(2);
    up=true;
    if (up) {
        aff_Rect(0,0,String::format("%3.0f degC",Meteo.Temperature));
        aff_Rect(0,1,String::format(" H = %2d %%",Meteo.Humidite));
        aff_Rect(1,1,String::format("%3.0f km/h",Meteo.Vitesse));
        aff_Rect(2,0,String::format("%4.0f hPa",Meteo.Pression));
        aff_Rect(1,0,"Vent: "+Meteo.Sens.substring(1,Meteo.Sens.length()-1));
        l = strlen(Meteo.ciel)-2;
        tft.setCursor(int(80.0 - l*6.0/2.0),102);tft.println(Meteo.ciel.substring(1,l+1));
        tft.setCursor(83,82);tft.println(Meteo.PressionTrend.substring(1,Meteo.PressionTrend.length()-1));
    } else {
        tft.setTextSize(1);
        tft.setCursor(10,65);tft.println(Meteo.Sens+String::format(" %3.0f km/h",Meteo.Vitesse));
        tft.setCursor(10,85);tft.println(String::format("%4.0f hPa",Meteo.Pression));
        l = 79 - strlen(Meteo.PressionTrend)/2*5;
        tft.setCursor(l,82);tft.println(Meteo.PressionTrend);
    }
}
void aff_Mosaic() {
    tft.fillRect(0,20,tft.width(),126,ST7735_BLACK);
    tft.fillRect(0,21,33,25,ST7735_YELLOW);tft.fillRect(0,48,33,14,ST7735_BLUE);tft.fillRect(0,63,33,30,ST7735_WHITE);tft.fillRect(0,96,33,30,ST7735_WHITE);
    tft.fillRect(33,21,30,25,ST7735_BLACK);tft.fillRect(33,48,30,14,ST7735_WHITE);tft.fillRect(33,63,30,30,ST7735_YELLOW);tft.fillRect(33,96,30,30,ST7735_BLUE);
    tft.fillRect(65,21,60,25,ST7735_YELLOW);tft.fillRect(65,48,60,14,ST7735_WHITE);tft.fillRect(65,63,60,30,ST7735_RED);tft.fillRect(65,96,60,30,ST7735_WHITE);
    tft.fillRect(126,21,11,25,ST7735_WHITE);tft.fillRect(126,48,11,14,ST7735_YELLOW);tft.fillRect(126,63,11,30,ST7735_BLUE);tft.fillRect(126,96,11,30,ST7735_BLACK);
    tft.fillRect(139,21,20,25,ST7735_RED);tft.fillRect(139,48,20,14,ST7735_RED);tft.fillRect(139,63,20,30,ST7735_WHITE);tft.fillRect(139,96,20,30,ST7735_YELLOW);
}
void aff_Trame() { // dashboard ...
  tft.fillScreen(ST7735_BLACK);
  tft.drawFastHLine(0,33,160,ST7735_WHITE);
  tft.drawFastHLine(0,73,160,ST7735_WHITE);
  tft.drawFastVLine(40,0,33,ST7735_WHITE);
  tft.drawFastVLine(80,0,33,ST7735_WHITE);
  tft.drawFastVLine(120,0,120,ST7735_WHITE);
  tft.setTextColor(tft.Color565(0x40,0x40,0x40),ST7735_BLACK);
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
}
void aff_Code(String message) { //affiche le code
  tft.setTextColor(ST7735_CYAN,ST7735_BLACK);
  tft.setTextSize(1);
  tft.setCursor(0,119);
  tft.println(message);
}
void aff_Compteur(float val, char *mesure, float r) {
    int valeur;    
    if (r==0.0) {
        valeur = 225 - (val / 100.0 * 270.0);
    } else {
        valeur = 225 - (r / 100.0 * 270.0);
    }
    aff_cls();
    aff_Titre(mesure);
    //    tft.fillRect(0,40,tft.width(),110,ST7735_BLACK);
    tft.drawRay(64,82,33,38,-45,225,6 ,tft.Color565(0xDC,0xDC,0xDC)); // 
    tft.drawRay(64,82,30,38,-45,225,27 ,ST7735_WHITE); // 
    tft.drawRay(64,82,24,38,valeur-2,valeur+2,1,ST7735_GREEN); // 
    tft.setCursor(45,75);
    tft.setTextSize(1);
    tft.setTextColor(ST7735_WHITE,ST7735_BLACK);
    tft.print(String::format("%4.1f",val));
}
void aff_Capteur() {
    aff_Rect(0,0,String::format("%3.0f degC",temperature));
    aff_Rect(0,1,String::format(" L = %4.0f",luminosite));
    aff_Rect(1,0,String::format("%4.0f hPa",pression));
    aff_Rect(2,0,String::format("%4.1f ",roll));
    aff_Rect(3,0,String::format("%4.1f ",pitch));
}
void aff_Titre(char* titre) {
    tft.fillRect(1,20,tft.width()-2,19,ST7735_BLUE);
    int l = strlen(titre);
    tft.setTextColor(tft.Color565(0xC0,0xC0,0xFF),ST7735_BLUE);
    tft.setTextSize(2);
    tft.setCursor(80-l*6,22); tft.println(titre);
}
#endif
//------------------------------------------------------------------ Web Information ------
// Get information depuis le site Accuweather
void getRequest() {     
  // serveur Accuweather
  Meteo.data = false;
  //  request.hostname = "dataservice.accuweather.com"; //IPAddress(192,168,1,169); 
  request.hostname = "apidev.accuweather.com"; //from internet
  request.port = 80;
  //request.path = "/currentconditions/v1/623.json?language=en&details=false&apikey=hoArfRosT1215"; //OU3bvqL9RzlrtSqXAJwg93E1Tlo3grVS";  
  String loc = String::format("%s",lieux[meteoS]);
  request.path = "/currentconditions/v1/"+loc+".json?language=fr-fr&details=true&apikey=hoArfRosT1215"; //from inetnet
  request.body = "";
  http.get(request, response, headers);
  /*
  tft.setCursor(129,78);
  tft.setTextColor(ST7735_WHITE,ST7735_BLACK);
  tft.setTextSize(1);
  tft.println(response.status);*/
  aff_Code(String::format("%d",response.status));
  //  Serial.println(request.url);
  //  Serial.println(response.status);
  //  Serial.println(response.body);
  if (response.status == 200) { 
    String key1 = "WeatherText";
    Meteo.ciel = KeyJson(key1 , response.body);
    Meteo.jour =  (KeyJson("IsDayTime" , response.body) == "false") ? false : true;
    String jsonTemp = KeyJson("Temperature" , response.body);
    Meteo.Temperature = atof(KeyJson("Value",jsonTemp).c_str());
    jsonTemp = KeyJson("RealFeelTemperature" , response.body);
    Meteo.Ressentie = atof(KeyJson("Value",jsonTemp).c_str());
    Meteo.Humidite = atoi(KeyJson("RelativeHumidity",response.body).c_str());
    jsonTemp = KeyJson("Wind" , response.body);
    Meteo.Direction = atoi(KeyJson("Degrees", jsonTemp).c_str());
    Meteo.Sens = KeyJson("Localized", response.body);
    jsonTemp = KeyJson("Speed" , response.body);
    Meteo.Vitesse = atof(KeyJson("Value" , jsonTemp).c_str());
    jsonTemp = KeyJson("Pressure" , response.body);
    Meteo.Pression = atof(KeyJson("Value",jsonTemp).c_str());
    jsonTemp = KeyJson("PressureTendency" , response.body);
    Meteo.PressionTrend = KeyJson("LocalizedText",jsonTemp);
    Meteo.data = true;
  }
  //  Particle.process();
 }  

String KeyJson(const String& k, const String& j){
  int keyStartsAt = j.indexOf(k);
  //  Serial.println( keyStartsAt );
  int keyEndsAt = keyStartsAt + k.length(); // inludes double quote
  //  Serial.println( keyEndsAt );
  int colonPosition = j.indexOf(":", keyEndsAt);
  int valueEndsAt = j.indexOf(",", colonPosition);
  String val = j.substring(colonPosition + 1, valueEndsAt);
  val.trim();
  //  Serial.println( val );
  return val;
}
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
    char *arg_s[10];
    int i=0;
    serial_on = Serial.isConnected();
    while (p != NULL)
    {
    //    strcpy(arg_s[i],p); 
        arg[i++] = atoi(p);// & 0xFF;
        p = strtok (NULL, " ,.-");
    }
    
    if (Cde.startsWith("coul"))  commande = 0xA0;
    if (Cde.startsWith("mess"))  commande = 0xB0;
    if (Cde.startsWith("on"))  commande = 0xF0;
    if (Cde.startsWith("auto"))  commande = 0xF1;
    switch (commande) {
        case 0xA0: // color,w,g,r,b
            commande  = arg[0] & 0xFF;
            Lamp_couleur = (arg[0] << 24) + (arg[1] << 16) + (arg[2] << 8) + arg[3];
            Lamp_auto = false;
            Lamp_on = true;
            sprintf(szMess,"Change la couleur wgrb : %X:%X:%X:%X",arg[0],arg[1],arg[2],arg[3]);
            break;
        case 0xB0: // affiche un message sur l'écran
            sprintf(szMessage,"%s",arg_s[0]);
            b_Message = true;
            break;
        case 0xF0:// On/Off de la lampe
            Lamp_on = !Lamp_on;
            sprintf(szMess,"Switch on/off %s",Lamp_on ? "on":"off");
            break;
        case 0xF1:// On/Off mode auto de la lampe
            Lamp_auto = !Lamp_auto;
            sprintf(szMess,"Switch mode auto %s",Lamp_auto ? "on" : "off");
            break;
        default:
            break;
    }
    if (serial_on) {
      Serial.println("WebCde : reception WebCommande -> ");
      Serial.println(szMess);
    }
    Particle.publish("Status", szMess);
    return commande;
} // end WebCde
//------------------------------------------------------------------ BOUTONS -----
void bouton1() {
    switch (mode) {
        case MODE_CYCLE:
            if (Lamp_on) {
                Lamp_w = (Lamp_w+1) % 13;
                Lamp_couleur = Lamp_l[Lamp_w] << 24;
                Lamp_color(Lamp_couleur,Lamp_mask);
            } else {
                tm_b_cycle = true; 
            }
            break;
        case MODE_LAMPE:
            break;
        case MODE_MENU:
            menuS = (menuS % menuC) +1;
            menu = (menu & 0xFFF0) + (((menu&0x000f) % menuC) + 1);
            //menu = (menu & 0xFF00) + (((menu & 0x00FF) + 1) % m_count);
            //wait_start = 0x00;
            break;
    }
}
void bouton2() {
    switch (mode) {
        case MODE_CYCLE:
            Lamp_auto = !Lamp_auto;
            Lamp_on = true;
            break;
        case MODE_LAMPE:
            // change couleur
            break;
        case MODE_MENU:
            menuO = menu;
            menu *= 0x10;
            //menu = (menu & 0xFF00) + (((menu & 0x00FF) + 1) % m_count);
            //wait_start = 0x00;
            break;
    }
}
void bouton3() {
    Lamp_on = !Lamp_on;
}
void bouton1L() {

}
//------------------------------------------------------------ MENU -------
void affMenu(short l, char* menutxt[] ) {
    aff_Entete();
    bool bis = false;
    if (l>5) bis=true;
    for (short i=0; i<l;printMenu(i++,menuS, &menutxt[i-1],bis));
}
void printMenu(short n, short s, char* menutxt[],bool bis) {
    short dx=0,dy=0;
    if (bis) {
        if (n>4) { dx=80;dy=5;}
        tft.fillRect(1+dx,((n-dy)*15)+40,tft.width()/2-2,13,(n==(s-1)) ? ST7735_BLUE : 0x5555);
        tft.setCursor(3+dx,((n-dy)*15)+42);
    } else {
        tft.fillRect(1,(n*15)+40,tft.width()-2,13,(n==(s-1)) ? ST7735_BLUE : 0x5555);
        tft.setCursor(3,(n*15)+42);
    }
    tft.setTextSize(1);
    tft.setTextColor(ST7735_WHITE);
    tft.print(*menutxt);
}
void aff_Rect(uint8_t n, uint8_t s, String szMess) {
        tft.fillRect(1+s*80,(n*20)+40,tft.width()/2-2,13, 0x5555);
        tft.setCursor(3+s*80,(n*20)+42);
        tft.setTextSize(1);
        tft.setTextColor(ST7735_WHITE);
        tft.print(szMess);
}
void getCompass() {
    //https://cdn-learn.adafruit.com/downloads/pdf/adafruit-10-dof-imu-breakout-lsm303-l3gd20-bmp180.pdf
  sensors_event_t event; 
  mag.getEvent(&event);
  float Pi = 3.14159;
  // Calculate the angle of the vector y,x
  float heading = (atan2(event.magnetic.y,event.magnetic.x) * 180) / Pi;
  // Normalize to 0-360
  if (heading < 0)
  {    heading = 360 + heading;}
  tft.setTextSize(1);
  tft.setCursor(110,110);tft.println(String::format("%4.0f",heading));
}
void getAccelgyro() {
    // read raw accel/gyro measurements from device
    gyro.getEvent(&event);
    f_gx = event.gyro.x;
    f_gy = event.gyro.y;
    f_gz = event.gyro.z;
    accel.getEvent(&event);
    f_ax = event.acceleration.x;
    f_ay = event.acceleration.y;
    f_az = event.acceleration.z;
    /*
    f_ax = ax *2.0 /32768.0 ;//- accelBias[0];// + 1.0; //add gravity ?
    f_ay = ay *2.0 /32768.0 ;//- accelBias[1];
    f_az = az *2.0 /32768.0 ;//- accelBias[2];
    f_gx = gx *250.0 /32768.0 - gyroBias[0];
    f_gy = gy *250.0 /32768.0 - gyroBias[1];
    f_gz = gz *250.0 /32768.0 - gyroBias[2];
    */
}
#define RAD_TO_DEG 180.0/M_PI
void updateYPR() {
    float apha = 0.93;  //0.96 ou 0.93 suivant plusieurs sources
        //conversion accel en angles
//            a_ay = -atan(f_ax/sqrt(pow(f_ay,2) + pow(f_az,2))) *180.0 / M_PI; // en deg
//            a_ax = atan(f_ay/sqrt(pow(f_ax,2) + pow(f_az,2)))*180.0 / M_PI;
//            a_az = atan((sqrt(pow(f_ax,2) + pow(f_ay,2)))/f_az)*180.0 / M_PI;
            a_ay = -atan(f_ax/sqrt(f_ay*f_ay + f_az*f_az)) * RAD_TO_DEG ; //180.0 / M_PI; // en deg -> pitch
            a_ax = atan(f_ay/sqrt(f_ax*f_ax + f_az*f_az))* RAD_TO_DEG; // -> roll
            a_az = atan((sqrt(f_ax*f_ax + f_ay*f_ay))/f_az)* RAD_TO_DEG; // -> yaw
//#ifdef RESTRICT_PITCH // Eq. 25 and 26
  roll = atan2(f_ay, f_az) * RAD_TO_DEG;
  pitch = atan(-f_ax / sqrt(f_ay * f_ay + f_az * f_az)) * RAD_TO_DEG;
//#else // Eq. 28 and 29
//  roll = atan(accY / sqrt(f_ax * f_ax + f_az * f_az)) * 180/M_PI;
//  pitch = atan2(-f_ax, f_az) * 180/M_PI;
//#endif
}
//---------------------------------------------------------------- SENSOR ---
void aff_Mag(void)
{
  sensor_t sensor;
  mag.getSensor(&sensor);
  aff_cls();
  aff_Titre("Magnetic");
  tft.setTextColor(tft.Color565(0xC0,0xC0,0xFF),ST7735_BLACK);
  tft.setTextSize(1);
  tft.setCursor(0,40);
  tft.println(String::format("Sensor: %s",sensor.name));
  tft.println(String::format("Version: %d",sensor.version));
  tft.println(String::format("Id: %d",sensor.sensor_id));
  tft.println(String::format("Max: %5.1f uT",sensor.max_value));
  tft.println(String::format("Min: %5.1f uT",sensor.min_value));
  tft.println(String::format("Resolution: %5.1f uT",sensor.resolution));
  aff_Entete();
}
void aff_Accel(void)
{
  sensor_t sensor;
  accel.getSensor(&sensor);
  aff_cls();
  aff_Titre("Accelerometre");
  tft.setTextColor(tft.Color565(0xC0,0xC0,0xFF),ST7735_BLACK);
  tft.setTextSize(1);
  tft.setCursor(0,40);
  tft.println(String::format("Sensor: %s",sensor.name));
  tft.println(String::format("Version: %d",sensor.version));
  tft.println(String::format("Id: %d",sensor.sensor_id));
  tft.println(String::format("Max: %5.1f m/s^2",sensor.max_value));
  tft.println(String::format("Min: %5.1f m/s^2",sensor.min_value));
  tft.println(String::format("Resolution: %5.1f m/s^2",sensor.resolution));
  aff_Entete();
}
void aff_Gyro(void)
{
  sensor_t sensor;
  gyro.getSensor(&sensor);
  aff_cls();
  aff_Titre("Gyroscope");
  tft.setTextColor(tft.Color565(0xC0,0xC0,0xFF),ST7735_BLACK);
  tft.setTextSize(1);
  tft.setCursor(0,40);
  tft.println(String::format("Sensor: %s",sensor.name));
  tft.println(String::format("Version: %d",sensor.version));
  tft.println(String::format("Id: %d",sensor.sensor_id));
  tft.println(String::format("Max: %5.1f rad/s",sensor.max_value));
  tft.println(String::format("Min: %5.1f rad/s",sensor.min_value));
  tft.println(String::format("Resolution: %5.1f rad/s",sensor.resolution));
  aff_Entete();
}