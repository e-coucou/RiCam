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
#define VERSION_MIN 3
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
void aff_Click(char * szMess[], uint16_t couleur = ST7735_RED);
// ---- Commande Web
int WebCde(String Cde);
bool serial_on;
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
uint8_t meteoS = 0;
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
    mag.begin();
  delay(3000);
  aff_Trame();
}
//---------------------------------------
// variables LOOP
volatile unsigned long now, start = 0, count =0, iteration=0;
volatile int update = 0;
uint32_t lumiere=0x0;
bool tft_update=true;
char szMessage[30];
sensors_event_t event;
double bme_t,bme_p;
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
      aff_Status(1,120,"...");
      getRequest();
      tm_b_aff = false;
  }
  //----------------------------------------------------------------- le Bouton -------------
    down.Update();
    if (down.clicks !=0) {
        Button = down.clicks;
        tm_cycle.reset();
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
                if (mode == MODE_MENU) {
                    mode = MODE_CYCLE;
                } else {
                    mode = MODE_MENU;
                    menuC = 5; menuS = 0; menu = 1;
                }
                break;
        }
        Button = 0x00;
        down.clicks = 0;
        if (mode == MODE_MENU) {
          aff_Status(0,120,String::format("[%d]",menu));
          switch (menu) {
            case 50: // Exit menu, retour au mode Cycle
                mode = MODE_CYCLE;
                tm_b_cycle = true;
            case 130:
                menuC = 5; menuS = 0; menu = 1;
            case 0 : menu=5;
            case 1 : // Capteurs
                aff_cls();
            case 2 : // Gyro
            case 3 : // Meteo
            case 4 : // Lampe
            case 5 : // Exit
                affMenu(menuC,&menu_principal[0]);
                break;
            case 10: 
                menuC = 4;
                aff_Click("Capteur 1"); break;
            case 11:    aff_Click("Capteur 2"); break;
            case 12:    aff_Click("Capteur 3"); break;
            case 13:    aff_Click("Retour (..)",ST7735_BLUE); break;
            default : //erreur on revient ancien menu car menu non implémenté
                menu = menuO;
                break;
          }
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
              if (Meteo.data) {
                  aff_Meteo(false);
              } else {
                aff_Status(50,10,"No data from Web !");
              }
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
        bmp.getEvent(&event);
        if (event.pressure)
        {
            float temperature;
            /* Display ambient temperature in C */
            bmp.getTemperature(&temperature);
            bme_t = double(temperature);
            bme_p = double(event.pressure);
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
                Particle.publish("Rky_T",String(bme_t),60,PUBLIC);
                break;
            case 0x09 :
                //Particle.publish("Rky_H",String(bme_h),60,PUBLIC);
                break;
            case 0x0A :
                Particle.publish("Rky_P",String(bme_p),60,PUBLIC);
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
    tft.println(String::format("RiCam (%d)",mode));
    tft.drawFastHLine(0,126,75,ST7735_BLUE);
    tft.drawFastHLine(75,126,10,ST7735_WHITE);
    tft.drawFastHLine(85,126,75,ST7735_RED);
    tft_update = false;
}
void aff_Date() {
    int jour = int(Time.day());
    int mois = int(Time.month());
    int annee = int(Time.year());
    tft.setTextColor(tft.Color565(0xA0,0xA0,0xF0),ST7735_BLACK);
    tft.setTextSize(3);
    tft.setCursor(51,26);tft.println(String::format("%s%d ",jour>9 ? "":"0",jour));
    tft.setCursor(51,49);tft.println(Mois[mois-1]);
    tft.setCursor(51,72);tft.println(String::format(" %2d",annee-2000));
    tft_update = false;
}
void aff_Click(char * szMess[],uint16_t couleur) {
    //    tft.fillRect(0,0,tft.width(),tft.height(),ST7735_BLACK);
    //    tft.setTextColor(tft.Color565(0xAF,0xEE,0xEE));
    tft.fillScreen(couleur);
    tft.setTextColor(ST7735_WHITE);
    tft.setCursor(10,50);
    tft.setTextSize(4);
    tft.println(szMess);
    tft_update = false;
}
void aff_cls(uint16_t couleur) { // Clear Screen Black)
    tft.fillScreen(couleur);// 19 19 70 - ST7735_BLACK);
}
void aff_Meteo(bool up) {
    int l;
    tft.setTextColor(tft.Color565(0xA0,0xA0,0xF0),ST7735_BLACK);
    tft.setTextSize(3);
    tft.setCursor(31,22);tft.println(menu_lieux[meteoS]);
    tft.setTextSize(1);
    tft.setCursor(140,22);tft.println(String::format("(%c)",Meteo.jour ? 'J':'N'));
    tft.setCursor(50,45);tft.println(lieux[meteoS]);
    tft.setTextColor(tft.Color565(0x80,0x80,0xC0),ST7735_BLACK);
    tft.setTextSize(2);
    if (up) {
        tft.setCursor(20,65);tft.println(String::format("%3.0f C %2d%%",Meteo.Temperature,Meteo.Humidite));
        l = strlen(Meteo.ciel);
        l = 79 - l/2*7;
        tft.setTextSize(1);
        tft.setCursor(l,85);tft.println(Meteo.ciel);
    } else {
        tft.setTextSize(1);
        tft.setCursor(10,65);tft.println(Meteo.Sens+String::format(" %3.0f km/h",Meteo.Vitesse));
        tft.setCursor(10,85);tft.println(String::format("%4.0f hPa",Meteo.Pression));
        l = 79 - strlen(Meteo.PressionTrend)/2*7;
        tft.setCursor(l,105);tft.println(Meteo.PressionTrend);
    }
}
void aff_Trame() {
  tft.fillScreen(ST7735_BLACK);
  //  tft.drawFastHLine(0,33,160,ST7735_WHITE);
  //  tft.drawFastHLine(0,73,160,ST7735_WHITE);
  //  tft.drawFastVLine(40,0,33,ST7735_WHITE);
  //  tft.drawFastVLine(80,0,33,ST7735_WHITE);
  //  tft.drawFastVLine(120,0,120,ST7735_WHITE);
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
  request.path = "/currentconditions/v1/623.json?language=fr-fr&details=true&apikey=hoArfRosT1215"; //from inetnet
  request.body = "";
  http.get(request, response, headers);
  /*
  tft.setCursor(129,78);
  tft.setTextColor(ST7735_WHITE,ST7735_BLACK);
  tft.setTextSize(1);
  tft.println(response.status);*/
  aff_Status(120,1,String::format("%d",response.status));
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
  Particle.process();
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
    String arg_s[10];
    int i=0;
    serial_on = Serial.isConnected();
    while (p != NULL)
    {
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
            sprintf(szMess,"%s",arg[0]);
            aff_Status(20,1,szMess);
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
            menuS = (menuS + 1) % menuC;
            menu = ((menu % menuC) + 1) % menuC;
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
            menu *= 10;
            //menu = (menu & 0xFF00) + (((menu & 0x00FF) + 1) % m_count);
            //wait_start = 0x00;
            break;
    }
}
void bouton3() {
    Lamp_on = !Lamp_on;
}
//------------------------------------------------------------ MENU -------
void affMenu(short l, char* menutxt[] ) {
    aff_Entete();
    size_t length = sizeof(menutxt); ///sizeof(*a)
    tft.setCursor(120,5);tft.println(String::format("%d",length));
    for (short i=0; i<l;printMenu(i++,menuS, &menutxt[i-1]));
}
void printMenu(short n, short s, char* menutxt[]) {
    tft.fillRect(1,(n*15)+40,tft.width()-2,13,(n==(s-1)) ? ST7735_BLUE : 0x5555);
    tft.setCursor(3,(n*15)+42);
    tft.setTextSize(1);
    tft.setTextColor(ST7735_WHITE);
    tft.print(*menutxt);
}
