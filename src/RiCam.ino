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
// Version Information
#define NAME "RiCam"
#define AUTEUR "eCoucou"
#define VERSION_MAJ 0
#define VERSION_MIN 2
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
// ---- Commande Web
int WebCde(String Cde);
bool serial_on;

String Mois[12] = {"JAN", "FEV", "MAR", "AVR", "MAI", "JUN", "JUI", "AOU", "SEP", "OCT", "NOV", "DEC"};
struct stParam {
  uint8_t version, resolution;
  unsigned long timeout;
};
stParam Param = {1,0,60000};

struct stAccuWeather {
  float Temperature,Vitesse;
  String ciel;
  String Sens;
  int Direction,Humidite;
  bool jour, data;
};
stAccuWeather Meteo;
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
uint32_t Lamp_couleur,Lamp_mask=0xFFFF,wait_start; // Neo_color : 0xWWGGRRBB
char tour=0;
char Lamp_w = 12;
bool Lamp_on = true;
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
Timer tm_aff(60000,Aff);
bool tm_b_aff = false;
//--------------------- timer CLOUD : every 20 sec. ---
void Cloud() {
    tm_b_cloud = true;
}
void Aff() {
    tm_b_aff = true;
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
  Time.zone(+2);
  Particle.variable("luminosite",luminosite);
  Particle.variable("illumination",illumination);
  Wire.setSpeed(CLOCK_SPEED_100KHZ);
  Wire.begin();
  lampe.begin();
  // Start WebCommande
  bool success = Particle.function("Cde",WebCde);
  // initialize SPI:
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
      aff_Heure();
      tm_b_aff = false;
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
//    sprintf(szMess,"%2d:%s%d",heure,minute>9 ? "":"0",minute);
//    tft.fillRect(0,0,tft.width(),tft.height(),ST7735_BLACK);
//    tft.setTextColor(tft.Color565(0xAF,0xEE,0xEE));
    tft.fillScreen(ST7735_BLACK);
    tft.setTextColor(ST7735_WHITE,ST7735_BLACK);
    tft.setTextSize(3);
    tft.setCursor(60,40);tft.println(String::format("%s%d",heure>9 ? "":"0",heure));
    tft.setCursor(60,65);tft.println(String::format("%s%d",minute>9 ? "":"0",minute));
    tft.setCursor(145,1);
    tft.setTextSize(1);
    tft.println(String::format("%s%d",seconde>9 ? "":"0", seconde));
    tft_update = false;
}
void aff_Seconde() { //128x160
    int seconde = int(Time.second());
    tft.setTextColor(ST7735_WHITE,ST7735_BLACK);
    tft.setCursor(145,1);
    tft.setTextSize(1);
    tft.println(String::format("%s%d",seconde>9 ? "":"0", seconde));
    tft_update = false;
}
void aff_Date() {
//    char szMess[20];
    int jour = int(Time.day());
    int mois = int(Time.month());
    int annee = int(Time.year());
//    sprintf(szMess,"%2d/%2d/%4d",jour,mois,annee);
//    tft.fillRect(0,0,tft.width(),tft.height(),ST7735_BLACK);
//    tft.setTextColor(tft.Color565(0xAF,0xEE,0xEE));
//    tft.fillScreen(ST7735_BLACK);
    tft.setTextColor(ST7735_WHITE,ST7735_BLACK);
    tft.setTextSize(1);
    tft.setCursor(50,37);tft.println(String::format("%2d-%s-%2d",jour,Mois[mois-1],annee));
//    tft.setCursor(137,37);tft.println(String::format("%2d",jour));
//    tft.setCursor(133,47);tft.println(Mois[mois-1]);
//    tft.setCursor(130,57);tft.println(String::format("%2d",annee));
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
//  tft.drawFastHLine(0,33,160,ST7735_WHITE);
//  tft.drawFastHLine(0,73,160,ST7735_WHITE);
//  tft.drawFastVLine(40,0,33,ST7735_WHITE);
//  tft.drawFastVLine(80,0,33,ST7735_WHITE);
//  tft.drawFastVLine(120,0,120,ST7735_WHITE);
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
}
#endif
//------------------------------------------------------------------ Web Commande ------
// Get information depuis le site Accuweather
void getRequest() {     
  // serveur Accuweather
  Meteo.data = false;
  request.hostname = "dataservice.accuweather.com"; //IPAddress(192,168,1,169); 
  request.hostname = "apidev.accuweather.com"; //from internet
  request.port = 80;
  request.path = "/currentconditions/v1/623.json?language=en&details=false&apikey=hoArfRosT1215"; //OU3bvqL9RzlrtSqXAJwg93E1Tlo3grVS";  
  request.path = "/currentconditions/v1/623.json?language=fr-fr&details=true&apikey=hoArfRosT1215"; //from inetnet
  request.body = "";
   
  http.get(request, response, headers);
  tft.setCursor(129,78);
  tft.setTextColor(ST7735_WHITE,ST7735_BLACK);
  tft.setTextSize(1);
  tft.println(response.status);
//  Serial.println(request.url);
//  Serial.println(response.status);
//  Serial.println(response.body);
  if (response.status == 200) { 
    String key1 = "WeatherText";
    Meteo.ciel = KeyJson(key1 , response.body);
    Meteo.jour =  (KeyJson("IsDayTime" , response.body) == "false") ? false : true;
    String jsonTemp = KeyJson("Temperature" , response.body);
    Meteo.Temperature = atof(KeyJson("Value",jsonTemp).c_str());
    Meteo.Humidite = atoi(KeyJson("RelativeHumidity",response.body).c_str());
    jsonTemp = KeyJson("Wind" , response.body);
//    Serial.println(jsonTemp);
    Meteo.Direction = atoi(KeyJson("Degrees", jsonTemp).c_str());
    Meteo.Sens = KeyJson("Localized", response.body);
    jsonTemp = KeyJson("Speed" , response.body);
    Meteo.Vitesse = atof(KeyJson("Value" , jsonTemp).c_str());
    Meteo.data = true;
  }
  if (Meteo.data) {
    Serial.println(Meteo.Humidite);
    Serial.println(Meteo.Temperature);
    Serial.println(Meteo.ciel);
    Serial.println(Meteo.Sens);
    Serial.println(Meteo.Direction);
    Serial.println(Meteo.Vitesse);
    Serial.println(String::format("-- %d / %d",int(('é')),int('a')));
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
