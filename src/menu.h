// Mode
#define MODE_CYCLE 0
#define MODE_LAMPE 1
#define MODE_MENU  2
//  Menu ----------------------
unsigned short menuL[2] = {0x0,0xB}; // c'est quoi ?
char menuS = 1;
char* menu_principal[] = { "Capteurs", "Lampe","Meteo", "Cycle" };