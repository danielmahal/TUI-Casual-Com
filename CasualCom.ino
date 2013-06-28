//#define DisplaySerial Serial

#include <SoftwareSerial.h>
SoftwareSerial DisplaySerial(10,11);

#include <Picaso_Const4D.h>
#include <Picaso_Serial_4DLib.h>

const unsigned int potPin = A0;

const unsigned int width = 320;
const unsigned int height = 240;
const unsigned int bgColor = BLACK;

int colors[] = {0xF7DF, 0x0451, 0xFB56, 0xCAEB};

int personRadius = 5;
float personDistance = height / 2 - personRadius;

int people[] = { 7, 9, 13, 18, 18 };
char peopleNames[][30] = { "Takeshi", "Daniel", "Jane", "Ritika", "Luke" };
int personIndex = 0;

int potValue;
int prevPotValue;

Picaso_Serial_4DLib Display(&DisplaySerial);

void setup() {
  Serial.begin(9600);
  
  pinMode(potPin, INPUT);
  
  Display.Callback4D = mycallback;
  DisplaySerial.begin(9600);
  Display.TimeLimit4D  = 5000;
  
  Display.gfx_ScreenMode(LANDSCAPE_R);
  
  clearScreen();
  
  while(!Display.file_Mount()) {
    Display.putstr("Not mounted");
    delay(200);
    Display.gfx_Cls();
    delay(200);
  }
  
  Display.txt_FontID(FONT1);
  
  Display.putstr("Mounted");
  Display.file_Dir("*.*");
  
  int font1 = Display.file_LoadImageControl("NoName2.da1", "NoName2.gc1", 1);
  
  Display.txt_FontID(font1);
  Display.txt_Height(1);
  Display.txt_Width(1);
  Display.putstr("DANIEL");
  
//  clearScreen();
//  drawTimezones();
}

void clearName() {
  int pad = personRadius * 4;
  int x1 = width / 2 - height / 2 + pad;
  int x2 = width / 2 + height / 2 - pad;
  int y1 = height / 2 - 10;
  int y2 = width / 2 + 10;
  
  Display.gfx_RectangleFilled(x1, y1, x2, y2, bgColor);
}

void setName(char* name) {
  clearName();
  
  int textWidth = Display.charwidth(name[0]) * strlen(name);
  int textHeight = Display.charwidth(name[0]);
  
  Display.gfx_MoveTo((width / 2) - (textWidth / 2), (height / 2) - (textHeight / 2));
  Display.txt_FGcolour(WHITE);
  Display.putstr(name);
}

void clearScreen() {
  Display.gfx_BGcolour(bgColor);
  Display.gfx_Cls();
}

void setTimezone(float time, boolean active) {
  Serial.print("Change timezone: ");
  Serial.print(time);
  Serial.println();
  drawTimezone(time, active ? MAGENTA : WHITE);
}

void clearTimezone(float time) {
  drawTimezone(time, bgColor);
}

void drawTimezone(float time, int color) {
  float angle = getTimezoneAngle(time);
  Display.gfx_CircleFilled(width / 2 + (sin(angle) * personDistance), height / 2 + (cos(angle) * personDistance), personRadius, color);
}

void drawTimezones() {
  for(int i = 0; i < sizeof(people) / sizeof(int); i++) {
    setTimezone(people[i], i == 0);
  }
}

float getTimezoneAngle(float time) {
  return -((time / 24.0) * TWO_PI) - PI;
}

int wrapIndex(int i, int imax) {
   return ((i % imax) + imax) % imax;
}

void loop() {
//  potValue = analogRead(potPin);
//  int diff = potValue - prevPotValue;
//  
//  if(abs(diff) > 40) {
//    int prevIndex = personIndex;
//    personIndex = wrapIndex(personIndex + (diff < 0 ? -1 : 1), sizeof(people) / sizeof(int));
//    
//    setTimezone(people[prevIndex], false);
//    setTimezone(people[personIndex], true);
//    setName(peopleNames[personIndex]);
//    
//    prevPotValue = potValue;
//  }
}

void mycallback(int ErrCode, unsigned char Errorbyte) {
  const char *Error4DText[] = {"OK\0", "Timeout\0", "NAK\0", "Length\0", "Invalid\0"} ;
  Serial.print(F("Serial 4D Library reports error ")) ;
  Serial.print(Error4DText[ErrCode]) ;
  if (ErrCode == Err4D_NAK)
  {
    Serial.print(F(" returned data= ")) ;
    Serial.println(Errorbyte) ;
  }
  else
    Serial.println(F("")) ;
  while(1) ; // you can return here, or you can loop
}
