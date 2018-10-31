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