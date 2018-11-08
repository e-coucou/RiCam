// Mode
#define MODE_CYCLE 0
#define MODE_LAMPE 1
#define MODE_MENU  2
//  Menu ----------------------
#define MENU_00 0x0000 //principal
#define MENU_01 0x0100 //menu meteo
unsigned short menuL[2] = {0x0,0xB}; // c'est quoi ?
uint8_t menuS = 1, menuC = 5;
uint16_t menu = 0, menuO = 0;
char* menu_principal[]  = {"Capteurs","Gyro", "Meteo","Lampe", "Cycle" };
char* menu_lieux[]      = {"Paris", "Antony","Paris XV","Lyon", "Lyon 6", "St-Tropez","Dompierre","Belleme"};
char* menu_meteo[]      = {"Lieux" , "GetData" , "Detail", "retour"};
char* lieux[] = {"623","133591","133790","171210","2606143","166176","142265","137401"};
//------
int16_t ax, ay, az;
int16_t gx, gy, gz;
int16_t gyro_temp;
float f_ax,f_ay,f_az;
float f_gx,f_gy,f_gz;
float a_ax,a_ay,a_az,a_gx=0,a_gy=0,a_gz=0,n_gx,n_gy,n_gz,last_gx=0,last_gy=0,last_gz=0;
float accelBias[3] = { 0.0, 0.0, 0.0 }; // gravity
float gyroBias[3] = {-3.30, 2.29, 1.34 }; // my own !!
double k_ax,k_ay,k_az; // angle corriger par filtre Kalman
//Kalman kx,ky,kz; // Kalman filter
double yaw, roll, pitch,heading;
double compassFiltre = 0.0;
sensors_vec_t   orientation;
bool b_refresh = false;